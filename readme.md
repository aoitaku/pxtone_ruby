---
title: Pxtone/Ruby
author: aoitaku
version: 0.0.3
---

# Pxtone/Ruby

Ruby からピストンコラージュ（&copy; [Studio Pixel](http://studiopixel.sakura.ne.jp/pxtone/index.html)）の楽曲ファイル（ .ptcop や .pttune ）を再生できるようにする拡張ライブラリです。ノイズファイル（ .ptnoise ）を読み込んで DXRuby などで再生することもできます。  
Ruby2.0.0-i386-mingw32 用です。ほかはビルドを試していないので分かりません。

ピストンコラージュのバージョンは 0.9.2.5 です。

## ビルド方法

- RubyInstaller for Windows から Ruby 2.0.0 (32bit) の devkit をダウンロードしてインストール
- pxtone_ruby のディレクトリで extconf.rb を実行
- Makefile が作られるので make を実行

## 使い方

bin フォルダにビルド済みの Pxtone.so があるので、これを任意の場所に置きます。  
pxtoneWin32.dll を実行するファイルと同じディレクトリにおいて、Pxtone.so を

```ruby
require_relative "Pxtone"
```

で読み込んで使います。

```ruby
Pxtone.load_tune("ファイル名") # ファイルを読み込む
Pxtone.play # 再生する
pos = Pxtone.stop # 停止する
Pxtone.play(pos) # 停止位置から再開する
```

詳細は [APIリファレンス](https://github.com/aoitaku/pxtone_ruby/wiki) を参照してください。


## サンプル

sample フォルダに入っています。  
DXRuby を使っているので、DXRuby をインストールする必要があります。  
実行するときは pxtoneWin32.dll にパスを通すか、pxtoneWin32.dll のあるディレクトリで Ruby を実行してください。

よく分からない人は単に bin フォルダ で

```
ruby ../sample/pxtone.rb
```

とすれば実行できます。


## 謝辞（敬称略）

[Studio Pixel](http://studiopixel.sakura.ne.jp/)

ピストンコラージュの配布元です。


[Project DXRuby](http://dxruby.sourceforge.jp/)

本ライブラリの製作にあたって、DXRuby および Ayame/Ruby のソースコードを参考にさせていただきました。


## ライセンス

Pxtone/Ruby は zlib/libpng ライセンスとします。ご自由にお使いください。  
再配布の際には pxtoneWin32.dll を含めても構いません。ただし、本ライブラリを使用して生じた一切の問題について Studio Pixel に問い合わせを行わないようにお願いします。

同梱のサンプルおよび楽曲データはパブリックドメインとします。

フォントは個別のライセンスに従ってください。


## 補足

Pxtone/Ruby は DXRuby とセットでの利用を想定していますが、単体で利用可能です。
ただし、依存 DLL の関係上 Windows 上でしか動作しないため、Ruby/SDL などと合わせて利用した場合も Windows 上での動作に限定されます。あらかじめご了承ください。

## 更新履歴
- 2014-09-11 0.0.3
  .ptnoise の読み込み / String, Arrayへの書き出しをサポート

- 2014-08-31 0.0.2  
  pause/resume 機能を追加  
  quality メソッドの名前の typo を修正  
  文字列型の戻り値が正しいエンコーディング情報を持つように修正  
  一部引数の型の不正を修正

- 2014-08-31 0.0.1  
  公開
