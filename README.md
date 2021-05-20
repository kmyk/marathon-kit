# kmyk/marathon-kit

## なにこれ

AtCoder Heuristic Contest 用のテンプレートリポジトリです。


## 使い方

### テストを並列実行する

```console
$ python3 scripts/measure.py [-c COMMAND]
```

オプションの完全なリストは `--help` を読んでください。


### 途中経過をお手軽に visualize する

解法コードを修正し、暫定解を標準エラー出力に `-----BEGIN-----` と `-----END-----` とで挟んだ以下のようなフォーマットで好きな回数だけ出力するようにしてください (例: [main.cpp](https://github.com/kmyk/marathon-kit/blob/main/main.cpp) + `-DVISUALIZE` オプションを付けてコンパイル)。

```
-----BEGIN-----
(ここに暫定解を出力)
-----END-----
```

そして以下のように実行してください。

```console
$ python3 scripts/visualize.py [-c COMMAND]
```

`vis/out.mp4` に動画が出力されます。

利用例: <https://twitter.com/kimiyuki_u/status/1387368706481659907>


### GitHub に push するたび自動でテストする

まず、[scripts/measure.py](https://github.com/kmyk/marathon-kit/blob/main/scripts/measure.py) と [.github/workflows/measure.yml](https://github.com/kmyk/marathon-kit/blob/main/.github/workflows/measure.yml) をあなたのリポジトリに追加してください。
リポジトリをまだ作っていない場合は、リポジトリのページ右上の [Use this template](https://github.com/kmyk/marathon-kit/generate) をクリックすることでも同じことができます。

その後、`.github/workflows/` の中身を適切に修正してください。
`Prepare tools/` のステップの URL やファイル名を問題に合わせて更新する必要があります。
C++ 以外の言語を利用している人は `Compile the code` のステップの修正も必要です。


## 注意

AtCoder Heuristic Contest の問題で与えられる公式のビジュアライザの仕様が過去の問題から変化した場合は、それに応じて必要な箇所を適切に修正して利用してください。

過去の問題の復習に利用するときも同様です。
過去の問題については [Release 一覧](https://github.com/kmyk/marathon-kit/releases)から当時の状態のスクリプトを入手することもできます。


## License

MIT License
