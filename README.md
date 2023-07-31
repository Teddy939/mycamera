# mycamera
primitive contrast auto focus implementation in C++ for Raspberry Pi with Arducam motorized focus camera.

カメラ開発 (組み込みソフト) の勉強のため、原始的なコントラストオートフォーカスを C++ で実装しました。Raspberry Pi 2 Model B と [Arducam の電動フォーカスカメラモジュール](https://www.arducam.com/product/arducam-imx219-auto-focus-camera-module-drop-in-replacement-for-raspberry-pi-v2-and-nvidia-jetson-nano-camera/)で動作を確認しました。

Raspberry Pi 2 を選択したのは、開発開始時にたまたま手元にあったためで、Raspberry Pi 3 や 4 でも動作すると思われます。

OpenCV ベースの実装です (cv::VideoCapture や cv::imshow などを使用)。

# セットアップ
## ハードウェア
Arducam の電動フォーカスカメラが前提です。

[Rasberry Pi 公式のカメラモジュール V2](https://www.raspberrypi.com/documentation//accessories/camera.html#hardware-specification) のセンサー部分を交換するタイプの[こちら](https://www.arducam.com/product/arducam-imx219-auto-focus-camera-module-drop-in-replacement-for-raspberry-pi-v2-and-nvidia-jetson-nano-camera/)で動作を確認しましたが、単独タイプの[こちら](https://www.arducam.com/product/raspberry-pi-camera-5mp-autofocus-motorized-focus-camera-b0176/)でもおそらく動作すると思います。

カメラモジュールとRaspberry Pi 基板との接続手順は、[公式](https://www.raspberrypi.com/documentation//accessories/camera.html#installing-a-raspberry-pi-camera)に従ってください。

## OS
1. [Raspberry Pi 公式の手順](https://www.raspberrypi.com/documentation/computers/getting-started.html#installing-the-operating-system)に従い、Raspberry Pi OS の初回ログインまで済ませてください。

1. `sudo apt update` の後に `sudo apt full-upgrade` を実行し、OS にインストール済みのパッケージを更新 (これは[公式手順](https://www.raspberrypi.com/documentation/computers/os.html#using-apt))

1. `sudo raspi-config`

1. raspi-config メニューの `3 Interface Options` -> `I1 Legacy Camera Enable/disable legacy camera support` を選択し、有効化 (警告が出るかもしれませんが、構わず進める)

1. リブート

## I2C の有効化
フォーカスモータ制御の指示は I2C で行います。
Linux カーネルに `i2c-dev` モジュールを組み込むスクリプトを実行します。
本スクリプトは [Arducam の github リポジトリ](https://github.com/ArduCAM/RaspberryPi/tree/master/Motorized_Focus_Camera#enable-i2c)からそのまま拝借しています。

1. `git clone https://github.com/Teddy939/mycamera.git`

1. `cd mycamera/scripts/`

1. `sudo chmod +x enable_i2c_vc.sh`

1. `./enable_i2c_vc.sh`

1. Y を押してリブート

## 依存パッケージのインストール
1. `sudo apt install libopencv-dev`

# ビルド
本リポジトリを clone して出来たディレクトリ (`mycamera/`) がカレントワーキングディレクトリである前提

1. `cmake -S . -B build`

1. `cmake --build build`

# 実行
本リポジトリを clone して出来たディレクトリ (`mycamera/`) がカレントワーキングディレクトリである前提

1. `cd build/test/`

1. `./mycamera`

# 使い方
起動するとライブビュー画面が立ち上がり、中央に青枠が表示されます。この枠に囲われた範囲のコントラスト (標準偏差で近似) を計算し、最大値となる位置にフォーカスを合わせるように動作します。

"f" キーを押すと AF が動作を開始し、動作中は枠線の色がグレーになります。最大コントラスト位置の検出に成功し、そこにフォーカスが合うと枠線が緑色になります。失敗すると枠線が赤色になります。成功・失敗に関わらず、1 秒立つと枠線が青色に戻ります。

十字キーの上下を押すとフォーカス位置をマニュアルで動かせます。

"q" キーで終了します (コンソールで Ctrl + C でも終了可)。

# 著作権表示的なもの (他者の権利への配慮)
`scripts/enable_i2c_vc.sh`、`include/arducam_vcm.h`、`lib/libarducam_vcm.so` は [Arducam の リポジトリ](https://github.com/ArduCAM/RaspberryPi/tree/master)から拝借しています。このリポジトリ内の成果物のライセンスは、BSD 三条項ライセンスです。

Arducam 社の素晴らしい成果に感謝します。

(`arducam_vcm` の "vcm" はボイスコイルモータの略？)

# 課題
## フォーカス精度
現状ではあまり良くないです。合う時はピシッとピントが合うのですが、結構外すことが多いです。動作を見ていると、一番ピントが合っているところでコントラスト値が最大になっていない時が多いように思われます。

これは、後述の遅延と関係しているかもしれません。

## 遅延
カメラの前で手を振るとわかりますが、結構遅延します。カメラの設定は 30 fps、フレームバッファーサイズは 4 で動かすようにしています。

フレームバッファーは FIFO キューだと思うので、メインループの毎ループで取れるフレームは、その時点から 4 フレーム前のはずです。

すると

* 最小で (1000 / 30) * 3 = 99.99 ミリ秒
  * カメラがフレームをバッファに格納した直後にバッファ読み出し
* 最大で (1000 / 30) * 4 = 133.33 ミリ秒
  * カメラが次のフレームをバッファに格納する直前にバッファ読み出し

の遅延した画像を表示していることになります。

cv::imshow 関数で画面表示するのに、さらなる遅延も加算されるはずです。

また、フォーカスモータを駆動して、各フォーカス位置でのコントラスト値を計算する時には、メインループの中で

1. フォーカス駆動
1. 次のループでコントラスト値を計算

としています。

ループで取れる画像は 4 フレーム前のため、フォーカス駆動した直後の画像に対してコントラスト値を計算しているつもりが、駆動前の画像に対してコントラスト値を計算している可能性があります。つまりフォーカス駆動とコンストラスト計算が同期しておらず、ある種の位相ズレのような状態になっている可能性があります。

この仮説が正しいとすると、フォーカス精度にも悪く影響していそうです。

## 速度
遅いです。市販のデジタルカメラやスマートフォン内蔵カメラとは比べ物になりません。

使用したカメラに使われているのは、おそらくボイスコイルモータだと思いますが、一度に大きくポジションを変えると「カチッ、カチッ」と音がするので、大きく動かす時は段階的にしています。モータの駆動と駆動の間は 66 ミリ秒の間隔をおいています ([Arducam 社のコード](https://github.com/ArduCAM/RaspberryPi/blob/4e9872db288e4bb48762c5ae225a51623d040862/Motorized_Focus_Camera/python/Autofocus.py#L76)を参考にしました)。

また、最大コントラスト検出のアルゴリズムとして、愚直にフォーカスの最小位置から最大位置まで間隔を荒くスキャンし (coarse scan)、一番コントラスト値が高かったところの周辺を、より細かい間隔でスキャン (fine scan) して、二段目のスキャンで最も高いコントラスト値の位置にフォーカスを決めています。

一段目のスキャンで、最小位置から最大位置までフォーカス位置を動かしているので遅いです。

当初は、スキャン開始時点のフォーカス位置から少しずらし、コントラスト値が上がったか下がったかでスキャン方向を決めるように実装していたのですが、スキャン方向の検出に失敗することが多かったので (前述の遅延の影響か、コントラスト値の定義として採用した関数 (標準偏差) がよくないか、など)、現状では愚直に最小から最大までスキャンしています。

## フレームレート
カメラは 30 fps の設定ですが、AF 処理側のメインループは 30 fps よりも小さいフレームレートで動いている可能性が高いです。

メインループ内の処理の概略は

1. フレーム取得
1. コントラスト値の計算
1. フォーカス駆動
1. 画像表示
1. 1/30 秒待機

です。5. で 1/30 秒待機するのは、カメラのフレームバッファー更新頻度よりも細かくループしないようにしているからですが、1. 〜 4. の処理時間があるので、メインループは 30 より小さい fps で動いているはずです (現状で計測はしていません)。

メインループの実行頻度を正確に 30 fps にしたければ、1/30 秒毎にタイマー割り込みを発生するようにし、そのイベント処理として 1. 〜 4. を実行する必要があります。また一連の処理時間が 1/30 秒を超えないようにする必要もあります。

# 次のステップ
## 精度、速度、遅延の改善
* タイマー割り込みでメインループとカメラを同期する
* フォーカス駆動とコントラスト値の計算を同期する
* バッファー取得の遅延をどうにかする (no idea)
* コントラスト値の定義をいろいろ変えてみる (現状は標準偏差)

## 組み込み開発のスキル獲得
現状の Raspberry Pi OS ベースの開発では、なんとなく組み込みっぽくない。

* ITRON 系 OS で実装 (ベアメタル Raspberry Pi で TOPPERS/FMP3 カーネルを動かす。参考は[この辺](https://domisan.sakura.ne.jp/article/rp_toppers/rp_toppers.html))
* ライブビューを液晶モジュールに出す (MIPI DSI 規格の把握)
  * ちなみに今回のカメラは MIPI CSI-2 接続のはず (イメージセンサー SONY IMX219 の[データシート](https://www.arducam.com/downloads/modules/RaspberryPi_camera/IMX219DS.PDF))

## 3A (AF、AE、AWB) の実装
今回は AF ですが

* 自動露出 (Auto Exposure: AE)
* 自動ホワイトバランス (Auto White Balance: AWB)

も実装すると、カメラ制御の基礎感覚が身に付くと思われます。

## 機能追加
* 静止画を撮れるようにする (SD カードに保存)
* 人物検出・瞳検出 AF の実装 (HOG などの古典アルゴリズムから、ディープラーニングによるものまで)
  * 今回使用したカメラ + Jetson nano で OpenPose 系の骨格検出からの瞳位置検出が出来るかどうか試す (近頃ソニーやキヤノンのカメラで、骨格検出をベースにしていると思われる AF を搭載したカメラが登場しているので)

## コードの可読性の改善
* リファクタしないとコードが汚い

# 参考
カメラ技術の基礎的事項を[スライド](https://docs.google.com/presentation/d/1HmpGusV4Bv9vF_dcC_rle8ZeofpjLloAQP801PG8H9A/edit?usp=sharing)にまとめています。
