# Principal

## 制約事項

* コミュニケーションは日本語で行う
* 無断で git add, git commit 実行しない
* nsf_playerディレクトリ以外のファイルは操作しない
* Principal.md は私の命令を検討する前に、毎回読み込んで最新をチェックして             

## 今の目標

既存のnofreandoはNESファイル前提のため、これをNSFに対応するのは大変であることが分かりました。
そのため、まずは純粋なPCでビルドできるNSFを解釈できる6502エミュレータを作って、動いたらESP32ようにもっていくことにしたいと思う。


## nsf_playerの仕様

nsf_playerディレクトリにその実装を作りたい。
ヘッダファイルとソースファイルは同じディレクトリに配置する

Cで実装する。

外部に公開するIFは、NSFのファイル読み込みと、INIT実行と、一定周期でPLAY呼び出す機能である。APUの書き込みは今はStubでよい。

出力ファイル名は、nsf_player
引数で、NSFを指定する。
ログで、フェッチしている命令の情報、そのときにCPUの状態がどうなるか判定する

ESP32に持っていくときにnofrendoのAPUに結合する。実装方法はnofrendoを参考にしてほしい。  

Unimplemented opcodeがでなくなるように実装してください。
基本的にnofrendoで実装されているopcodeはすべて実装してください。

サブルーチン対応する。

## 問題があった時の対処方針

適当な代替で問題を隠蔽したりせずに、根本原因を明らかにして対処すること。
対処が難しい場合は、問い合わせること。

## サンプルファイル

temp/dq.nsf

これをエラー無く再生することが目標
サンプルファイルは、NSFプレーヤーでも正常に再生されるので、サンプルファイル事態に問題があることを疑ってはならない。

## nsf_play向けのビルド

makefileを使う

コンパイラはgccを使う


## ESP32向けのビルド

```bash
./.devcontainer/run_devcontainer.sh idf.py build
./.devcontainer/run_devcontainer.sh idf.py flash
```

* Claude codeではmonitorの実行できないので、行わない。

