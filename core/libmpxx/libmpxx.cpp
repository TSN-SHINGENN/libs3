/*! \mainpage libmpxxリファレンスガイド
 *
 * \section 概要
 * libmpxxはLinux/MACOSX/Windows/Solaris/QNX/AdobeActionScript等の異なるOSで提供されるAPIの仕様を吸収するための静的ライブラリです。<br>
 * アプリケーション作成時、異なる開発環境やOSでも一つのソースコードで実行バイナリを作成することを支援します。<br>
 * 以下の利点を考慮しC言語で書かれています。<br>
 * &nbsp; ・多くの関数をネイティブ近い形でサポートしています。<br>
 * &nbsp; ・個性のあるライブラリはlibmpxx内部での使用を避けています。よって、JAVAや.net等の他言語との接続を容易に行えます。<br>
 * &nbsp; &nbsp; ※ libmpxxの関数を他言語から直接呼び出す場合は必要な環境を用意してください。<br>
 * 
 * \section 背景
 * CPUネイティブコードでコンパイルし、高速で複数のOSに対応するためのライブラリとして誕生しました。 <br>
 * 大手メーカーの覇権争いで、囲い込みとして使われるプログラム開発言語にたいしてアルゴリズムをシェアするために開発されています。<br>
 * 近年ではマイコンへの高度なアルゴリズムの実装や、デザインパターンを用いたマルチコアCPU/GPUでの高速化等も実現しています。
 * このライブラリは統合化され、libs3の一つのコンポーネントとして製品開発に用意しています。
 *
 * \section 内容
 * &nbsp; ・標準化されているPOSIXやCランタイムライブラリでOS毎の差異や不足分を補います。<br>
 * &nbsp; ・コンパイラにより効率化・最適化されないx86 CPUの高度な命令についても利用可能なライブラリを提供します。<br>
 * &nbsp; &nbsp; 当然 旧x86 CPUに搭載されてない命令を使用することになりますが極力ソフトウェアでのエミュレーションが可能なように整備します。<br>
 * &nbsp; ・マルチコアCPUの恩恵を利用しやすいコードを容易にコーディングするためのライブラリを提供します。<br>
 * &nbsp; ・COOL(C-language's Object Oriented Language)でのコーディングを支援するためにSTL等の標準化さたライブラリをエミュレートする互換ライブラリを提供します。<br>
 * <br>
 * 
 * \section 対応OS
 * &nbsp; ・QNX 6.5.0 x86 gcc(QNX6.4.0以降でコンパイルが通ります) <br>
 * &nbsp; &nbsp; libmpxxをリンクする際に次のリンクオプション( -lm -lsocket)を追加してください。<br>
 * &nbsp; ・MACOSX 10.5.x/10.6.x /10.7.x/10.8.x/10.9.x/10.10.x gcc ※ 最新版での検証が追いついていません。<br>
 * &nbsp; &nbsp; MACOSX10.5系は32bit版のみになります。これは-mdynamic-no-picがコンパイルオプションに必須ですが、64bitではうまく機能しませんでした。
 * &nbsp; &nbsp; MACOSX10.5系のgccがSSE4系命令に非対応のため、直接使っている関数はエラーになります。<br>
 * &nbsp; &nbsp; &nbsp; しかし、この時SSE4を使う関数(crc32c等）はソフトウェアエミュレーションになります。<br>
 * &nbsp; &nbsp; MACOSXアプリケーションにlibmpxxをリンクする際には、次のリンクオプション( -lm -lpthread -framework Carbon)を追加してください。<br>
 * &nbsp; ・Windows XP SP 3/Vista/7/8/8.1  Microsoft Visual C++ 10.x/14.x/15.x <br>
 * &nbsp; &nbsp; *WIN32のネイティブなデスクトップアプリケーション開発で使用可能です。<br>
 * &nbsp; &nbsp; *MinGWについては利用実績はあります。しかしながら全てを検証していません。<br>
 * &nbsp; &nbsp; MinGWでの開発はVC++最新のランタイムを推奨します。とりあえずmsvcrt.dll(VC++6.0ランタイム）でも使用可能なように構成してあります。<br>
 * &nbsp; &nbsp;  ※ 確認の取れていないエラーがあるかもしれません。 <br>
 * &nbsp; &nbsp; &nbsp; 以下の説明はMinGWの旧版での使用方法です。新版ではこの限りではありません。<br>
 * &nbsp; &nbsp; &nbsp; QTなど、MinGWと連携する開発環境の場合も同様です。<br>
 * &nbsp; &nbsp; &nbsp; VC++10.0ランタイムを使用する場合はgccにオプションを付けてください。。<br>
 * &nbsp; &nbsp; &nbsp; (1)コンパイルオプションに -D__MSVCRT_VERSION__=0x1000 を追加してください。<br>
 * &nbsp; &nbsp; &nbsp; (2)gccのspecファイルを出力し以下の箇所を修正して、リンク時のライブラリを変更してください。<br>
 * &nbsp; &nbsp; &nbsp; &nbsp; -lmoldname  → -lmoldname100<br>
 * &nbsp; &nbsp; &nbsp; &nbsp; -lmsvcr → -lmsvcr100<br>
 * &nbsp; &nbsp; &nbsp; &nbsp; その他実行バイナリ生成に必要なライブラリを調査してリンクしてください。<br>
 * &nbsp; ・Linux (Fedora系 / SUSE系 / CentOS系で確認) gcc <br>
 * &nbsp; ・AdobeActionScript Crossbridge開発環境（ただし、コンパイルが通るようになったのみで全関数未検証）<br>
 * &nbsp;   バーチャルマシンでの実行となるため SSE命令等は使えません。libmpxxビルド時に対応する関数は排除されます<br>
 * &nbsp; &nbsp; libmpxxをリンクする際に次のリンクオプション( -lm -lpthread)を追加してください。<br>
 * &nbsp; ・FreeRTOS
 * &nbsp;   組み込みリアルタイム環境の時流に従い、検討していきます。
 * &nbsp; ・Xilinx Standaloneカーネル<br>
 * &nbsp;   上記カーネルではlibmpxxのかなりのPOSIX互換関数が使えないように制限されます。<br>
 * &nbsp;   Standaloneカーネルではスレッドをサポートしていないためスレッド系のライブラリは使えません。
 * &nbsp;   使えない関数は、使えないことを次の方法で警告します。<br>
 * &nbsp;   (1)リンク時に定義されていないことを警告し、コンパイルエラーになります。<br>
 * &nbsp;   (2)コンパイル時にENOTSUPを戻す。<br>
 * &nbsp;   (3)コンソールにメッセージを表示し、abort()を実行して処理をとめます。<br>
 * &nbso;   標準ではHEAPメモリがデフォルトで使えないように制限されます。メモリの節約のためだと思われます<br>
 * &nbso;   しかし、libmpxxはstatic領域を使って簡易的なメモリ領域を管理するためのアロケーターを用意しています。<br>
 * &nbso;   mpxx_lite_mallocater APIを使用して、ヒープ領域を使わずに処理することが可能です。<br>
 * &nbso;   mpxxライブラリの関数を使用するには mpxx_lite_mallocater_init()を使用してメモリを割り当ててください。<br>
 * &nbsp; 　makefile中の定義で、OS標準のメモリ管理機構に切り替えることも可能です。<br>
 * &nbsp; 　<br>
 * &nbsp; 　libmpxxプロジェクト使用する場合は Makefile Project with Existing Codeでソースコードを追加するフォルダを作成します。<br>
 * &nbsp; 　libmpxxソースコードを先のフォルダにコピーしてください。<br>
 * &nbsp; 　よってlibmpxxソースコードはバージョン管理機構から分岐し,てPlatformSDKに併合して管理します<br>
 * &nbsp; 　プロジェクトのC/C++Build->builder SettingでGenerate Makefile automatically 及び　Usedefault bild commandのチェックをはずします。<br>
 * &nbsp;   Build command の欄に次のコマンドで実行するように命令を書き変えます。<br>
 * &nbsp; &nbsp; Microblaze Standalone     : "make MICROBLAZE=1 XILINX=1 STANDALONE=1"<br>
 * &nbsp; &nbsp; Microblase_MCS Standalone : "make MICROBLAZE_MCS=1 XILINX=1 STANDALONE=1"<br>
 * &nbsp; 　プロジェクトのC/C++Build->Behaviourのbuild(Incrimental build)が"all"になっているのを書き換えます。<br>
 * &nbsp;   Release : "all" -> "release"<br>
 * &nbsp;   Debug   : "all" -> "debug"<br>
 * &nbsp;   libmpxxはビルド時に-ffunction-sections -fdata-sectionsオプションを用いて、リンク時に不要な関数をそぎ落とす前処理がされています。<br>
 * &nbsp;   elfファイルのメインとなるプロジェクトには リンク時に-Wl,-gc-sectionオプションが付加され、スリムなファイルが生成されることを確認してください。<br>
 * &nbsp;   不必要な関数はelfファイルのメインとなるプロジェクトには リンク時に-Wl,-gc-sectionオプションが付加され、スリムなバイナリファイルが生成されることを確認してください。<br>
 * &nbsp;   追加のハードウェアが必要なAPIは検証されておらず、ライブラリビルドから除外されています。必要に応じて追加してください。<br>
 * &nbsp; ※ Solarisは開発休止中です <br>
 *<br>
 * \section 対応CPU
 * &nbsp; ・主に Intel MMX Pentium 以降 x86系互換CPU（AMD64/EM64Tを含みます） <br>
 * &nbsp; &nbsp; ※ 低機能なx86互換CPUにも、対応できますがご相談ください。<br>
 * &nbsp; ・QNX6.5以降ではARM系 リトルエンディアンにも対応します。 <br>
 * &nbsp; ・AdobeActionScript Crossbridge VitualMachine<br>
 * &nbsp; ・Xilinx Microblaze(MCS含む）リトルエンディアン  及び ARM CoreリトルエンディアンとXPSの組み合わせ
 * &nbsp; &nbsp; ※ POSIX系OSについての他のCPUについてはご相談ください。SIMD以外なら使える可能性があります。 <br>
 * &nbsp; &nbsp; &nbsp; ARMにもSIMD命令はあるようですがlibmpxxに実装しないといけません。 <br>
 * &nbsp; &nbsp; ※ Sparc系については SunOS系の対応の開発休止をVW部で決めたので現在未対応です。 <br>
 *
 * \section 注意事項
 *
 * libmpxxは社内でアプリケーション及びライブラリを開発するために使用されます。<br>
 * 単体での社外公開はしていません。<br>
 * このライブラリは単体での公開は基本できません。
 * 理由はglibcなどのオープンソースライセンスなライブラリをコアとしてリンクする場合はライセンス違反になる可能性があります。
 * "異なるOSで提供されるAPIの仕様を吸収する"特性上、ラッピングするのみでは二次創作物といえません。この時、ライセンスに抵触する可能性があります。<br>
 * ※ 開発した独創的なアプリケーション・ライブラリは二次創作物となります。<br>
 * ※ ラッピング以外のエミュレート関数等は独自実装ですので公開可能です。ご相談ください。<br>
 *
 * \section 提供するファイル及び関数について
 *
 * \section リポジトリ情報
 *
 * 秘密です。<br>
 * 
 * \section 著作権
 * &nbsp; Copyright(C) TSN･SHINGENN
 * <br>
 * &nbsp; &nbsp; mpxx_getopt()の内部実装であるown_getopt_exec()はAT&T社パブリックドメインライセンスとなっているgetopt()の拡張版となっています。<br>
 * <br>
 * \section 利用について
 *	このソースコードおよびコンパイル済みバイナリの利用は、開発者に全ての権利を有します。許可なく組み込むことはできません。
 */

/**
 * @file libmpxx.c
 * @brief libmpxxライブラリの情報取得関数
 */

/* this */
#include "mpxx_endian.hpp"
#include "libmpxx_revision.h"
#include "libmpxx.h"

/**
 * @fn const char *const mpxx_get_lib_revision(void)
 * @brief libmpxxの版管理番号の取得
 * @return 版管理番号文字列ポインタ
 */
const char *mpxx_get_lib_revision(void)
{
    return LIB_MPXX_REVISION;
}

/**
 * @fn const char *const mpxx_get_lib_name(void)
 * @brief libmpxxライブラリの標準名を返します
 * @return 標準名文字列ポインタ
 */
const char *mpxx_get_lib_name(void)
{
    return LIB_MPXX_NAME;
}

const char *const _libmpxx_idkeyword =
    "@(#)libmpxx_idkeyword : " LIB_MPXX_NAME " revision"
    LIB_MPXX_REVISION
    " CopyRight (C) TSN･SHINGENN All Rights Reserved.";

/**
 * @fn const char *mpxx_get_idkeyword(void)
 * @brief libmpxx_idkeywordを返します。
 * @return idkeyword文字列ポインタ
 */
const char *mpxx_get_idkeyword(void)
{
    return _libmpxx_idkeyword;
}
