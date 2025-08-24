# Principal

## 制約事項

* コミュニケーションは日本語で行う
* 無断で git add, git commit 実行しない
* Principal.md は私の命令を検討する前に、毎回読み込んで最新をチェックして             

## 今の目標

PicoRubyで、logformat.txt の仕様に従ったファイルを読み込み、APUクラスで再生することです。

## 問題があった時の対処方針

適当な代替で問題を隠蔽したりせずに、根本原因を明らかにして対処すること。
対処が難しい場合は、問い合わせること。

## ESP32向けのビルド

```bash
./.devcontainer/run_devcontainer.sh idf.py build
./.devcontainer/run_devcontainer.sh idf.py flash
```

* Claude codeではmonitorの実行できないので、行わない。

