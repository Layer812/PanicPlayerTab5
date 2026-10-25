#!/usr/bin/env python3
"""Standalone R5Y FIX4 9-source reconstruction from pinned X68K Tab, isolated build only.

No writes to development trees, production trees, GitHub, ESP32-P4 or flash.
"""
from __future__ import annotations
import argparse
import csv
import datetime as dt
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import traceback
import zipfile

HERE = Path(__file__).resolve().parent.parent
PIN = 'b8a5f29c747f4345060b32a34f190f9339d5b577'
REPOSITORY = 'https://github.com/Layer812/PanicPlayerTab5.git'
REPORT_NAME = 'PANIC_R5Y_PUBLIC_SOURCE_BUILD_SEND_THIS.zip'
SHARED_GIT_BLOBS = {
    'src/main.c': '42ea68167ee649e2e9c3fb951ecfb141f1fb6bdb',
    'src/tab5_panic.cpp': 'bd148679d47c98e5e2d19726040a75a9e6ea1c74',
    'src/tab5_panic.h': '25fda04f25c5eca9a9422acc923c34e5a0f815bd',
    'src/tab5_screen_manager.c': '00d550eff61fbcee1cb12ef8bf30e2232757d661',
    'components/px68k/x68k/hostfs.c': '29741204c961634a0078b142925950a1451af8ca',
    'components/px68k/x68k/palette.c': '2072d4b91dff8e098768127f5e46ad7548b75692',
    'components/px68k/x68k/sysport.c': 'c1d437c80cc6b3f858a94dc1cc9fa128f92ee76c',
    'src/CMakeLists.txt': '3d2d4a64c33250a5d2a17adc490abb389b5d73dd',
    'src/panic_x_bin.c': '662b334b73edc2f5c973194078a78d04d6365cab',
    'src/tab5_video.h': '712aa33106cfa53466d76dece023a4684a20ce2f',
    'src/tab5_video.cpp': 'ede4302fd40bfdc6b59cc855b406797cc09935f6',
}


def sha256_bytes(b: bytes) -> str:
    return hashlib.sha256(b).hexdigest()


def git_blob(b: bytes) -> str:
    return hashlib.sha1(b'blob ' + str(len(b)).encode('ascii') + b'\x00' + b).hexdigest()


def safe_rel(value: str) -> Path:
    p = Path(value)
    if p.is_absolute() or '..' in p.parts or '\\' in value or p.parts[0] in ('', '.'):  # manifest paths use forward slashes
        raise ValueError('Unsafe manifest path: '+value)
    return p


def new_stage(root: Path) -> Path:
    root.mkdir(parents=True, exist_ok=True)
    prefix = 'R5Y_RECURSIVE_BUILD_' + dt.datetime.now().strftime('%Y%m%d_%H%M%S')
    for i in range(1, 100):
        p = root/(prefix + (('_%02d' % i) if i > 1 else ''))
        try:
            p.mkdir()
            return p
        except FileExistsError:
            pass
    raise RuntimeError('Could not allocate distinct build stage')


def run(command, cwd: Path | None, logfile: Path, env=None):
    command = [str(x) for x in command]
    line = '$ ' + subprocess.list2cmdline(command)
    print(line, flush=True)
    with logfile.open('a', encoding='utf-8', errors='replace') as f:
        f.write('\n'+line+'\n')
        f.flush()
        p = subprocess.Popen(command, cwd=cwd, env=env, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT, text=True, errors='replace',
                             bufsize=1)
        for part in p.stdout:
            print(part, end='', flush=True)
            f.write(part)
        rc = p.wait()
        f.write('\n[EXIT] '+str(rc)+'\n')
    if rc != 0:
        raise RuntimeError('Command failed (%d): %s' % (rc, line))


def output(args, cwd: Path) -> str:
    return subprocess.check_output([str(x) for x in args], cwd=cwd, text=True, errors='replace').strip()


def hash_preflight():
    manifest = json.loads((HERE/'SHA256_MANIFEST.json').read_text(encoding='utf-8'))
    if manifest['pin_x68ktab'] != PIN or len(manifest['files']) != 9:
        raise RuntimeError('Manifest is not the 9-file R5Y overlay')
    verified = []
    for rec in manifest['files']:
        rel=safe_rel(rec['path'])
        src=HERE/'overlays'/rel
        if not src.is_file() or src.is_symlink():
            raise RuntimeError('Missing or symlink overlay: '+str(src))
        b=src.read_bytes()
        actual=sha256_bytes(b)
        if actual != rec['sha256']:
            raise RuntimeError('Damaged R5Y source: '+str(rel))
        verified.append({'path':str(rel).replace('\\','/'),'sha256':actual,'bytes':len(b)})
    return verified


def upstream_sources_guard(shared: Path, report: Path):
    """Verify immutable Git blobs *and* a clean checkout, regardless of core.autocrlf.

    GitHub's blob SHA-1 is over repository contents, not necessarily Windows
    worktree bytes. On Windows with core.autocrlf=true, tracked LF source can
    be checked out as CRLF; hashing those raw bytes produces a false mismatch.
    `git rev-parse HEAD:path` identifies the canonical committed blob; `git diff
    HEAD -- path` verifies the tracked file wasn't modified after checkout.
    Both guards are mandatory, and neither allows an unexpected upstream pin.
    """
    records=[]
    for path, expected in SHARED_GIT_BLOBS.items():
        target=shared/safe_rel(path)
        if not target.is_file() or target.is_symlink():
            raise RuntimeError('Pinned upstream target missing or symlink: '+path)
        actual=output(['git','rev-parse','HEAD:'+path],shared).lower()
        if actual!=expected:
            raise RuntimeError('Pinned upstream Git blob mismatch: '+path+
                               ' expected='+expected+' actual='+actual)
        clean=subprocess.run(['git','diff','--quiet','--exit-code','HEAD','--',path],
                             cwd=shared,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        if clean.returncode!=0:
            raise RuntimeError('Pinned upstream checkout modified or diff failed: '+path+
                               ' (rc='+str(clean.returncode)+') '+
                               clean.stderr.decode('utf-8',errors='replace').strip())
        raw=target.read_bytes()
        records.append({'path':path,'committed_git_blob':actual,
                        'checkout_unchanged':True,'worktree_bytes':len(raw),
                        'worktree_sha256':sha256_bytes(raw),
                        'git_blob_of_raw_worktree_bytes':git_blob(raw),
                        'worktree_equals_git_blob':git_blob(raw)==expected})
        print('PASS upstream Git blob + clean checkout: '+path,flush=True)
    (report/'UPSTREAM_SOURCE_GUARD.json').write_text(
        json.dumps({'commit':PIN,'guard':'Git HEAD blob + unmodified checkout',
                    'status':'PASS','files':records},indent=2),
        encoding='utf-8')
    return records


def add_compiled_sources(s: str) -> str:
    if '"tab5_panic.cpp"' in s or '"panic_x_bin.c"' in s:
        raise RuntimeError('Unexpected CMake baseline: PANIC targets already registered')
    for token in ('"tab5_launcher.cpp"', '"tab5_media_ui.cpp"'):
        if s.count(token) != 1:
            raise RuntimeError('Unexpected CMake baseline: '+token)
    return s.replace('"tab5_launcher.cpp"', '"tab5_launcher.cpp"\n        "tab5_panic.cpp"')\
            .replace('"tab5_media_ui.cpp"', '"panic_x_bin.c"\n        "tab5_media_ui.cpp"')


def reuse_verified_checkout(stage: Path, log: Path) -> Path | None:
    """Reuse a prior immutable integration clone after verifying refs and origin.

    The source build lives in a *new* stage. Reuse avoids redownloading large
    nested dependencies, but never trusts unverified checkout contents.
    `upstream_sources_guard` runs again on the selected checkout afterwards.
    """
    expected_modules={'M5GFX':'729297d6e3d657ddc1ec5189bac2f2ea68828085',
                      'M5Unified':'4fb444784c85791e0b0207701392b42be234b2e7'}
    for c in sorted(stage.parent.glob('R5Y_RECURSIVE_BUILD_*/checkout'),reverse=True):
        if c==stage/'checkout' or not (c/'X68KTab5').is_dir():
            continue
        try:
            origin=output(['git','remote','get-url','origin'],c)
            if origin.lower().removesuffix('.git') != REPOSITORY.lower().removesuffix('.git'):
                continue
            shared=c/'X68KTab5'
            if output(['git','rev-parse','HEAD'],shared)!=PIN:
                continue
            if any(output(['git','rev-parse','HEAD'],shared/'components'/key)!=value
                   for key,value in expected_modules.items()):
                continue
            # Confirm source paths exist and are not symlinks before selecting.
            if any(not (shared/safe_rel(path)).is_file() or
                   (shared/safe_rel(path)).is_symlink()
                   for path in SHARED_GIT_BLOBS):
                continue
            log.parent.mkdir(parents=True,exist_ok=True)
            with log.open('a',encoding='utf-8') as f:
                f.write('REUSE VERIFIED LOCAL RECURSIVE CHECKOUT: '+str(c)+'\n')
            print('REUSE VERIFIED LOCAL RECURSIVE CHECKOUT: '+str(c),flush=True)
            return c
        except (OSError,subprocess.SubprocessError,ValueError):
            continue
    return None


def build_source(stage: Path, report: Path, verified: list):
    log=report/'INTEGRATION.log'
    source=stage/'source'
    # Prefer the repository's already recursively initialized submodule. This
    # avoids cloning a second copy and preserves the exact Git-link revision.
    local_checkout = HERE if (HERE/'X68KTab5').is_dir() else None
    if local_checkout is not None:
        origin=output(['git','remote','get-url','origin'],local_checkout)
        if origin.lower().removesuffix('.git').rstrip('/') != REPOSITORY.lower().removesuffix('.git').rstrip('/'):
            raise RuntimeError('Unexpected source repository origin: '+origin)
        log.parent.mkdir(parents=True,exist_ok=True)
        with log.open('a',encoding='utf-8') as f:
            f.write('REUSE LOCAL RECURSIVE GIT SUBMODULE FROM PUBLIC REPOSITORY: '+str(local_checkout)+'\n')
        checkout=local_checkout
    else:
        checkout=reuse_verified_checkout(stage,log)
    if checkout is None:
        checkout=stage/'checkout'
        run(['git','clone','--recurse-submodules',REPOSITORY,str(checkout)],None,log)
    shared=checkout/'X68KTab5'
    if output(['git','rev-parse','HEAD'],shared) != PIN:
        raise RuntimeError('Unexpected X68KTab5 submodule revision (pin mismatch)')
    expected_nested = {'M5GFX':'729297d6e3d657ddc1ec5189bac2f2ea68828085',
                       'M5Unified':'4fb444784c85791e0b0207701392b42be234b2e7'}
    for name in ('M5GFX','M5Unified'):
        m=shared/'components'/name
        if not m.is_dir() or not (m/'src').is_dir():
            raise RuntimeError('Missing recursive nested submodule: '+name)
        head=output(['git','rev-parse','HEAD'],m)
        if head != expected_nested[name]:
            raise RuntimeError('Unexpected nested module commit '+name+': '+head)
        print('VERIFIED nested module %s HEAD=%s' % (name,head),flush=True)
    upstream_sources_guard(shared, report)
    # Copy the shared source, but keep nested submodules as separate git worktrees.
    # Required by patch_m5gfx_x68ktab.py, which explicitly verifies M5GFX's git HEAD.
    def ignore(folder,names):
        excluded={'.git','build','sdkconfig.old'}
        folder=Path(folder)
        if folder==shared/'components':
            excluded.update({'M5GFX','M5Unified'})
        return set(names)&excluded
    shutil.copytree(shared,source,ignore=ignore,symlinks=False)
    for name in ('M5GFX','M5Unified'):
        m=shared/'components'/name
        pinned=output(['git','rev-parse','HEAD'],m)
        dest=source/'components'/name
        run(['git','worktree','add','--detach',str(dest),pinned],m,log)
        if output(['git','rev-parse','HEAD'],dest)!=pinned:
            raise RuntimeError('Nested worktree does not match checked-out pin: '+name)
    for record in verified:
        rel=safe_rel(record['path'])
        target=source/rel
        if not target.is_file() or target.is_symlink():
            raise RuntimeError('Unexpected overlay target: '+str(target))
        shutil.copyfile(HERE/'overlays'/rel,target)
        if sha256_bytes(target.read_bytes())!=record['sha256']:
            raise RuntimeError('Failed to copy exact R5Y source: '+str(rel))
    cmake=source/'src'/'CMakeLists.txt'
    cmake.write_text(add_compiled_sources(cmake.read_text(encoding='utf-8')),encoding='utf-8',newline='\n')
    if not all(('"'+x+'"') in cmake.read_text(encoding='utf-8') for x in ('tab5_panic.cpp','panic_x_bin.c')):
        raise RuntimeError('Missing PANIC build targets')
    validate_r5y_interfaces(source)
    (report/'SOURCE_RECONSTRUCTION.json').write_text(json.dumps({
        'base_repo':REPOSITORY,
        'pinned_x68ktab_commit':PIN,
        'overlays':verified,
        'cmake_src_sha256':sha256_bytes(cmake.read_bytes()),
        'source_staged':str(source),
        'warning':'This is a candidate integration, not proven equivalent to the R5Y hardware binary.',
    },indent=2,ensure_ascii=False),encoding='utf-8')
    return source


def validate_r5y_interfaces(source: Path):
    """Do not silently compile with old X68K Tab video UI."""
    header=(source/'src'/'tab5_video.h').read_text(encoding='utf-8')
    cpp=(source/'src'/'tab5_video.cpp').read_text(encoding='utf-8')
    for name in ('TAB5_VIDEO_ACTION_PANIC_SPACE',
                 'TAB5_VIDEO_ACTION_PANIC_QUEST',
                 'TAB5_VIDEO_ACTION_PANIC_REPEAT',
                 'TAB5_VIDEO_ACTION_PANIC_BACK',
                 'TAB5_VIDEO_ACTION_PANIC_PREV',
                 'TAB5_VIDEO_ACTION_PANIC_NEXT',
                 'tab5_video_turbo_mode',
                 'tab5_video_host_ui_active'):
        if name not in header:
            raise RuntimeError('R5Y video header missing: '+name)
    for name in ('tab5_video_set_panic_controls_enabled',
                 'tab5_video_begin_host_ui',
                 'tab5_video_end_host_ui',
                 'tab5_video_host_ui_active',
                 'TAB5_VIDEO_ACTION_PANIC_BACK',
                 'tab5_video_cycle_turbo_mode'):
        if name not in cpp:
            raise RuntimeError('R5Y video implementation missing: '+name)


def native_idf_build_command(idf_path: str, python_executable: str | None = None) -> list[str]:
    """Return a Windows-safe command for the real ESP-IDF build.

    `idf.py` is a Python entrypoint; on Windows it is not a PE executable.
    `subprocess.Popen(['idf.py','build'])` can fail with WinError 193 even after
    export.bat correctly activated the IDF environment.  Pass the Python
    interpreter explicitly, never shell=True, and use the activated IDF tree.
    """
    idf = Path(idf_path)
    if '5.5.4' not in str(idf).replace('\\','/').lower():
        raise RuntimeError('Unexpected IDF_PATH (ESP-IDF 5.5.4 required): '+str(idf))
    script = idf/'tools'/'idf.py'
    if not script.is_file() or script.is_symlink():
        raise RuntimeError('ESP-IDF tools/idf.py is missing: '+str(script))
    python = python_executable if python_executable is not None else sys.executable
    if not python or not Path(python).is_file():
        raise RuntimeError('Active Python interpreter is missing: '+str(python))
    return [str(python),str(script),'build']


def pack_report(report: Path, report_out: Path):
    report_out.parent.mkdir(parents=True, exist_ok=True)
    # Always one clearly named report in F:\build\tmp (without silently deleting a prior report).
    if report_out.exists():
        old=report_out.with_name(report_out.stem+'_PREVIOUS_'+dt.datetime.now().strftime('%Y%m%d_%H%M%S')+'.zip')
        report_out.rename(old)
    with zipfile.ZipFile(report_out,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=8) as z:
        for p in sorted(report.iterdir()):
            if p.is_file():
                z.write(p,p.name)
    with zipfile.ZipFile(report_out) as z:
        if z.testzip() is not None: raise RuntimeError('Report zip failed CRC check')
    print('\nONE REPORT: '+str(report_out),flush=True)


def main():
    p=argparse.ArgumentParser()
    p.add_argument('--public-root',default=r'F:\build\tmp\PanicPlayerTab5_Public')
    p.add_argument('--report-out',default=r'F:\build\tmp\PANIC_R5Y_PUBLIC_SOURCE_BUILD_SEND_THIS.zip')
    a=p.parse_args()
    root=Path(a.public_root)
    result={'version':'R5Y PUBLIC SOURCE / FIX4 NATIVE BUILD RECIPE','status':'SAFE STOP','stage':None,'build':'NOT ATTEMPTED','flash':'NOT ATTEMPTED',
            'github_publication':'NOT PERFORMED','upstream_pin':PIN}
    stage=None
    try:
        stage=new_stage(root)
        result['stage']=str(stage)
        report=stage/'report'
        report.mkdir()
        verified=hash_preflight()
        result['overlay_hash_check']='PASS'
        src=build_source(stage,report,verified)
        result['upstream_blob_and_checkout_guard']='PASS'
        result['source_integration']='PASS'
        # ESP-IDF environment must have been loaded by the CMD wrapper.
        idf=os.getenv('IDF_PATH','')
        if not idf or '5.5.4' not in idf.replace('\\','/').lower():
            raise RuntimeError('ESP-IDF 5.5.4 environment was not activated by CMD')
        result['build']='RUNNING'
        # Windows cannot directly CreateProcess() a Python-script `idf.py`.
        # Use the activated ESP-IDF environment's Python interpreter and
        # the exact IDF_PATH/tools/idf.py entrypoint instead.
        command = native_idf_build_command(idf)
        (report/'BUILD_LAUNCH.json').write_text(json.dumps({
            'launcher': 'python-interpreter + ESP-IDF tools/idf.py',
            'python': str(sys.executable),
            'idf_path': str(Path(idf)),
            'idf_py': command[1],
            'working_directory': str(src),
            'argv': command,
        },indent=2,ensure_ascii=False),encoding='utf-8')
        run(command,src,report/'NATIVE_BUILD.log')
        result['build']='PASS'
        result['status']='PASS (native compile only; no flashing, no hardware claim)'
        return_code=0
    except Exception as ex:
        result['error']=str(ex)
        result['traceback']=traceback.format_exc()
        traceback.print_exc()
        if stage is not None:
            (stage/'report'/'ERROR_TRACEBACK.txt').write_text(result['traceback'],encoding='utf-8')
        if result['build']=='RUNNING':result['build']='FAIL'
        return_code=2
    finally:
        if stage is not None:
            report=stage/'report'
            report.mkdir(exist_ok=True)
            (report/'RESULT.json').write_text(json.dumps(result,indent=2,ensure_ascii=False),encoding='utf-8')
            try:
                pack_report(report,Path(a.report_out))
            except Exception as e:
                print('ERROR packaging report:',e,flush=True)
                return_code=2
        print('RESULT: '+result['status']+'; native build: '+result['build'],flush=True)
    return return_code


if __name__=='__main__':
    sys.exit(main())
