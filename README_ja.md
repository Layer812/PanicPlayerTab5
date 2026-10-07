<p align="center">
  <img src="000000.png" alt="panicplayer" width="100%">
</p>

# panicplayer

[English](README_en.md)

**panicplayer** は、M5Stack Tab5で **X68000のPANICデータ（`.PAN`）** を再生するためのPANICプレイヤーです。

PANICは、X68000上でグラフィック・スプライト・テキスト・音声などを組み合わせたアニメーションを再生するために使われていたシステムです。  
オリジナルの再生器 `panic.x` と `.PAN` データによって、当時さまざまな作品が作られていました。

panicplayerは汎用X68000エミュレータを目指したものではなく、**PANICを気軽に再生するためのX68000互換環境**として作っています。

## M5Stack Tab5

**M5Stack Tab5** は、ESP32-P4（RISC-V）、5インチ 1280×720 IPSタッチディスプレイ、32MB PSRAM、microSDカードスロット、スピーカーなどを備えた開発端末です。

この構成を使って、PANIC専用のX68000互換再生環境を動かしています。

- [M5Stack Tab5 公式ドキュメント](https://docs.m5stack.com/ja/core/Tab5)

## M5Burner

現在は **M5Burner** からインストールできます。

- [M5Burner 公式ページ](https://docs.m5stack.com/ja/uiflow/m5burner/intro)
- [M5Stack 公式ダウンロードページ](https://docs.m5stack.com/ja/download)

**M5Burner Share Code**

```text
WGUybBrwDw4naV65
```

M5Burnerの **Share Burn** から上記コードを入力してください。

## 使い方

microSDカードに `.PAN` ファイルを入れて起動し、PANファイラーから再生したいデータを選びます。

- `.PAN` ファイルをタップ → 再生
- **BACK** → PANファイラーへ戻る
- **PREV / NEXT** → 前 / 次のPANデータへ移動
- **REPEAT** → SPACE待ちをするPANデータを一定間隔で自動送り
- **TURBO** → NORMAL / GREEN / RED の負荷プロファイルを切り替え
- 再生中に画面をタップ → `SPACE`

TURBOはX68000側の再生速度そのものを変えるものではなく、Tab5側の映像・音声処理の負荷バランスを切り替える機能です。選択したモードは起動中そのまま維持されます。

## もっとX68000を楽しみたい方へ

panicplayerはPANIC再生に特化したプレイヤーです。M5Stack Tab5でもっと汎用的なX68000環境を楽しみたい方は、**X68K Tab** もどうぞ。

- [X68K Tab](https://github.com/Layer812/X68KTab5)

## PANIC V1.38について

panicplayerでは、再生環境の一部としてオリジナルの **PANIC再生器 `panic.x` V1.38** のバイナリを使用しています。

PANIC V1.38に付属していたオリジナルドキュメントも、このリポジトリの `docs/` に収録しています。

オリジナル配布ページ：

- [X68000 LIBRARY - PANIC](http://retropc.net/x68000/software/movie/panic/panic/)

V1.38付属ドキュメントによると、

- PANICの原作者は **ぱこたん / pako こと 永田英哉さん**
- `panic.x` V1.38は、pakoさん作の V1.34を元に **なしみさん** が改変
- PANICの著作権は **永田英哉 (pako)さん** が保有
- 原作者の宣言により、PANICは利用・配布・改造・商用利用について広く許可
- 個々の `.PAN` データについては、それぞれのデータ作者の方針に従う

とされています。

**pakoさん、なしみさん、PANICの開発・資料作成・配布・データ制作に関わった皆様に感謝いたします。**

古いHDD、MO、CD-RなどにPANICデータが残っていたら、ぜひpanicplayerで再生してみてください。  
もし動かないデータがあれば、再現方法やログと一緒に知らせていただけると助かります。

## ソースコード

ソースコードは今後公開予定です。

まだ調整中の部分もあるため、まずはM5Burner版でお試しください。

## Special Thanks

**Nochiさん**

## 著作権・謝辞

### SHARP X68000

X68000は **シャープ株式会社** が開発したコンピュータです。

panicplayerでは、**シャープ・プロダクツ・ユーザーズ・フォーラム（FSHARP）** を通して無償公開されたX68000用システムソフトウェアを、その当時の使用許諾条件に従って利用しています。

適用されるオリジナルの許諾条件は、本リポジトリの以下のファイルに原文のまま収録しています。

[`LICENSE_SHARP_X68000.txt`](LICENSE_SHARP_X68000.txt)

使用・再配布条件については、必ずこの原文をご参照ください。panicplayerは無償で配布しています。

SHARPソフトウェアのオリジナル公開ページ：

- [X68000 LIBRARY - SHARP ソフトウェア](http://retropc.net/x68000/software/sharp/)

SHARP、X68000、および関連するソフトウェア、名称、商標等の権利は、シャープ株式会社および各権利者に帰属します。

panicplayerは個人による非公式プロジェクトであり、**シャープ株式会社とは関係なく、同社による承認・協賛を受けたものではありません。**

### PANIC

PANIC / `panic.x` の著作権は、オリジナル配布文書の記載どおり **永田英哉 (pako / ぱこたん)さん** に帰属します。

`panic.x` V1.38には **なしみさん** による改変が含まれています。

V1.38のオリジナルドキュメントについても、当時の作者の説明・配布条件をそのまま参照できるよう本リポジトリに収録しています。

### Musashi

panicplayerでは、68000 CPUの実行コアとして **Karl Stenerudさんの Musashi**（Motorola 680x0エミュレーションエンジン）を使用しています。

**Karl Stenerudさん、およびMusashiの開発に関わった皆様に感謝いたします。**

- [Musashi 公式リポジトリ](https://github.com/kstenerud/Musashi)
- [サードパーティー表記](THIRD_PARTY_NOTICES.md)

Musashiの著作権表示および許諾文は `THIRD_PARTY_NOTICES.md` に収録しています。

### PX68K

panicplayerのX68000互換部分の実装では、**PX68K** の実装およびハードウェア挙動を参考にしています。

**hissoriiさんをはじめ、WinX68k / xkeropi / PX68Kへと続く開発・移植・資料整備に関わった皆様に感謝いたします。**

- [PX68K 公式リポジトリ](https://github.com/hissorii/px68k)
- [サードパーティー表記](THIRD_PARTY_NOTICES.md)

panicplayerは独立したプロジェクトであり、PX68Kの公式移植版ではありません。

### M5Stack

M5Stack、M5Stack Tab5、M5Burnerは **M5Stack Technology Co., Ltd.** の製品・サービスです。  
panicplayerはM5Stackとは独立したプロジェクトです。
