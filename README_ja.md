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
UjzO1MYxSNIeYBiu
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

PanicPlayerTab5固有のソースコードは現在このリポジトリでは公開していません。X68000互換環境の基盤コード・実装の詳細・第三者ライセンスについては [X68K Tab](https://github.com/Layer812/X68KTab5) を参照してください。X68K Tabは別プロジェクトであり、そのリポジトリだけで本プレイヤーの同一版を再現できるという意味ではありません。

## Special Thanks

**Nochiさん**

## 著作権・謝辞

- **Panic Player Tab5の新規コード**：© 2026 Layer812
- **PANIC / `PANIC.X`**：原作者・著作権者 **永田英哉（pako／ぱこたん）さん**、V1.38改訂 **なしみさん**。原版の文書・条件を優先します
- **Musashi**：© Karl Stenerudおよび貢献者（MIT License）
- **PX68K / WinX68k / xkeropi**：X68000エミュレータの開発・移植・調査に携わった皆様
- **各 `.PAN` 作品**：映像・音楽・データの権利は各制作・権利者に帰属します
- **SHARP X68000**：SHARPおよび各権利者の名称・商標。非公式・非公認の個人プロジェクトです

**PANIC本体の配布許諾と、PANデータの配布許諾は別です。** 詳細・一次資料は [THIRD_PARTY_NOTICES.md](https://github.com/Layer812/PanicPlayerWeb/blob/main/THIRD_PARTY_NOTICES.md)、[LICENSE_PANIC_X.txt](https://github.com/Layer812/PanicPlayerWeb/blob/main/LICENSE_PANIC_X.txt)、[PANIC_V1.38_NOTICE.txt](https://github.com/Layer812/PanicPlayerWeb/blob/main/PANIC_V1.38_NOTICE.txt) を参照してください。PANICのオリジナル文書は [PanicPlayerTab5のdocs](https://github.com/Layer812/PanicPlayerTab5/tree/main/docs) に保存されています。[SHARP/FSHARPの原許諾条件](LICENSE_SHARP_X68000.txt) と [X68K Tabの第三者ライセンス詳細](https://github.com/Layer812/X68KTab5/blob/main/THIRD_PARTY_NOTICES.md) も参照してください。

**Panic Player WebのMITライセンスは、Layer812が権利を持つWeb版の新規実装部分にのみ適用されます。** 本Tab5リポジトリや第三者のコード・実行形式・各PANへ自動的に適用されるものではありません。各素材の原許諾条件を優先します。

PANICとX68000の文化を支え、貴重なデータを保存されてきた皆様に感謝します。
