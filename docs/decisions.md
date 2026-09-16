# 豊洲チャレンジ 設計・作業の取り決め

最終更新: 2026-09-16

このドキュメントは、実装を進めるうえで合意した方針・決定事項・未決事項を記録する。
新しい決定が出たら、その都度ここに追記する。

---

## 1. 作業の進め方

### コードは必ず自分の手で書く
- Claude はコードを提示し、設計意図を説明するのみ。
- Claude はファイルの作成・編集ツールを使わない（このドキュメントのような
  ドキュメント整形を除く）。
- 学習目的のプロジェクトであり、コードを書く過程そのものが目的に含まれるため。

### 1ファイルずつ進める
1. そのファイルの役割を説明する（何を担当し、何を担当しないか）
2. 実装内容を箇条書きで列挙する（定義する型・関数、方針、他ファイルとの依存）
3. **ここで止まり、確認を待つ**
4. 「OK」が出てからコードを提示する
5. 書き終えて次に進むと言うまでは、次のファイルに進まない

例外: CMakeLists.txt 3ファイルは相互に関係するため、まとめて提示してよい。

### 推測で補完しない
指示にない部分を勝手に実装しない。判断が必要な箇所は「未決事項」に明記されて
いるので、そこに触れる必要が生じたら質問する。

---

## 2. 基本方針: TC2025 準拠

TC2025 (`~/TC2025`) は実機で動作実績のあるコードである。

> **TC2025 に存在しない機能・フィールド・抽象化を追加しない。**
> 便利そうに見えても、実績のない要素を増やすことはリスクである。

TC2025 と異なる実装を提案する場合は、必ずその旨を明記し、理由を説明した
うえで判断を仰ぐ。勝手に「改善」しない。

### 意図的に TC2025 から変更している点（議論の結果、合意済み）

| # | 変更内容 | 理由 |
|---|---|---|
| 1 | パーサを継承チェーンから平坦な static 関数へ | yaml-cpp 採用に伴う |
| 2 | 各ストリームの property を使わない | パラメータは config の property に集約する |
| 3 | ストリームの create を producer 側で行う | 各プロセスが自分の出力だけを create する |
| 4 | 単位を SI 統一 (m, rad, s) | TC2025 は mm/m 混在で事故のもとだった |
| 5 | CMake を add_subdirectory 方式へ | TC2025 はルートと各モジュールの二重管理で、C++標準が食い違っていた |
| 6 | ディレクトリを `include/` と `src/` に分割 | 公開ヘッダと内部ヘッダの境界を構造で表現するため。TC2025 は全モジュール `src/` のみ |
| 7 | 構造体を `struct X { };` 形式で宣言 | TC2025 は `typedef struct { } X;`。C++ では冗長で、無名型ゆえの制約もあるため（詳細は §3） |
| 8 | 型の命名を `xxx_param` / `config_data` に統一 | TC2025 は `Robot_info` / `LiDAR_info`、ダミー型は `config`。`config` はターゲット名・変数名と衝突しうるため避ける |
| 9 | サブ構造体の分割を「読み手プロセス単位」にする | TC2025 はセンサ・機能単位で、融合率を `Control_info` に入れるなど使用者と無関係な切り方だった |

---

## 3. 設計原則

### 単位
- すべて SI 単位 (m, rad, s)。
- センサドライバが mm を返す場合、そのプロセス内で m に変換してから SSM に載せる。

### 座標系
- SSM 上を流れる pose (odom_gl / ndt_gl / estim_gl) は、地図座標系・車輪中心基準で統一。
- 生センサデータ (urg_fs) はセンサ座標系のまま流す。取り付けオフセットの補正は
  consumer (ndt) 側で行う。
- 座標変換を行う関数は、offset がどちらの座標系から見た値かを必ずコメントで明記する。
  符号ミスは動作してしまい発見が遅れるため。

### 構造体
- 宣言は **`struct X { ... };`** 形式にする（`typedef struct { ... } X;` は使わない）。
  TC2025 は typedef 形式だが、C++ では冗長。加えて typedef 形式は構造体が無名になるため
  前方宣言ができず、テンプレート引数に渡せるのも C++11 以降の規定に依存する
  （`SSMApi< config_data, config_property >` が該当）。
- `#pragma pack` は使わない（全ストリームで統一。DSSM による他機通信の予定がないため）。
- 配列の添字は `_X` / `_Y` / `_YAW` の定数で参照する。
- char 配列は必ず固定長にする。SSM は構造体を共有メモリにそのままコピーするため、
  `std::string` を入れると内部ポインタがコピーされ、別プロセスでは無効なアドレスになる。

### ヘッダの置き場所（2026-09-16 決定）

> **外部に公開するものだけ `include/` に含める。それ以外は `.cpp` と一緒に `src/`。**

`include/` と `src/` の分割が表現しているのは置き場所ではなく **API 境界**である。
「このディレクトリのヘッダは他モジュールが使ってよい／こちらは内部実装」という宣言。

| ヘッダ | 誰が include するか | 置き場所 |
|---|---|---|
| `utility.hpp` | 全プロセス | `utility/include/` |
| `config.hpp` | 全プロセス（型定義を受け取る） | `config/include/` |
| `param.hpp` | `config.cpp` だけ | `config/src/` |

`param.hpp` を `include/` に置くと「他プロセスから使ってよいヘッダ」と誤解させる。
実際には yaml-cpp のパーサを隠蔽するためにあえて外へ出していないヘッダなので、
`src/` に置く方が設計意図と一致する。

新しいヘッダを作るたびに「これは他プロセスが使うか?」を判断すること。
**迷ったら `src/` に置く。** 後から `include/` へ移す方が、公開してしまったものを
引っ込めるより安全。

### コメント規約（2026-09-16 決定）

**説明コメントは原則1行まで。** 2行以上必要になったら、コメントを増やすのではなく
関数名や構造を見直す合図とする。コメントが多いと可読性が落ちるため。

| 場所 | 書き方 |
|---|---|
| ファイル先頭 | 1行で「このファイルが何を担当するか」 |
| 関数の直前 | 自明でないものだけ1行。特に**引数の向き・単位・失敗時の戻り値** |
| 構造体メンバ・定数 | 行末に単位を書く（TC2025 の config.hpp と同じ流儀） |
| Doxygen タグ | **使わない** |
| コメント記法 | **`//` を使う**（`/* */` は使わない）。TC2025 も `//` が主体（`//` 4527 箇所 : `/* */` 565 箇所） |

判断基準は「関数名を読めば分かることは書かない。読んでも分からないことだけ書く」。
TC2025 も Doxygen を使っていない（`@brief` は 0 ファイル）。SSM 本体は使っているが、
そちらに合わせる必要はない。

### コードスタイル
- **制御文には原則 `{ }` を付ける。ただし本文が `if` と同じ行に収まる場合は省略してよい**
  （2026-09-16 決定。一度「例外なし」で決めたが、実際に両方を書き比べた結果、
  単純代入まで 3 行に展開すると可読性が落ちすぎるため例外を認めた）。

| 本文の置き方 | `{ }` |
|---|---|
| `if` と同じ行に収まる（単純代入など） | 省略してよい |
| 改行する（複数行、`for` を含むなど） | **必須** |

  - 波括弧が防ぐのは「本文を次の行に置き、後から 2 文目を足してインデントに騙される」形
    （Apple の goto fail が実例）。同一行なら 2 文目を足す余地が無く、守るものがない。
  - TC2025 にも同一行 if は計 411 箇所あり、うち 258 が `return`/`continue`/`break` などの
    脱出ガード、153 が代入・関数呼び出し（例: `if( obp.pos.v < 0.3 ) obp.pos.v = 0.3;`）。
    ※ 当初「代入を伴う同一行 if は無い」と記録していたが誤りだったため訂正済み。
  - 波括弧では防げない類もある。TC2025 の `if( obp.pos.w < M_PI/4 ) obp.pos.v = M_PI/4;` は
    w を見て v に代入しているバグと思われ、コピペして片方だけ直し忘れた形。こちらは
    キー名とメンバ名の対応を上から確認する習慣で潰す。
- `.cpp` 内部の補助関数はすべて `static` で隠蔽する。
- `namespace` は使わない（SSM はプロセス分離されており、シンボル衝突の心配がない）。
- 状態を持たない処理はクラスにせず、関数として実装する。

---

## 4. 実行環境（実測値・2026-09-16 時点）

| 項目 | 実測 | 備考 |
|---|---|---|
| OS | Ubuntu 24.04.5 LTS | 当初想定は 22.04 だった |
| CMake | 3.28.3 | |
| PCL | 1.14 (`/usr/include/pcl-1.14`) | `find_package(PCL 1.11 REQUIRED)` は通る |
| yaml-cpp | 0.8.0 | **エクスポート名は `yaml-cpp::yaml-cpp`**。素の `yaml-cpp` ターゲットは 0.8 で廃止 |
| libssm | `/usr/local/lib64` | `ld.so.conf.d/rpp.conf` 登録済みで `-lssm` は通る |
| SSM ヘッダ | `/usr/local/include/ssm.hpp` | |

### SSM ヘッダを読んで分かったこと

- `SSMApi< T, P >` は `class SSMApi : public SSMApiBase` として定義されている（ssm.hpp:562）。
- `open()` / `setVerbose()` / `getStreamName()` / `getStreamId()` は `SSMApiBase` の
  **public 非テンプレート**メンバ。
- `SSMApi` のコンストラクタが `setBuffer( &data, sizeof(T), &property, sizeof(P) )` を
  呼ぶため、`SSMApiBase&` に落とした後もバッファサイズ情報は保持される。
  → **`openWithRetry()` は `SSMApiBase&` で受けられる**（テンプレート関数にする必要がない）。
- SSM には `openWait( timeOut, openMode )` という類似機能が既にある（1秒間隔固定）。
  今回は「何回目か表示したい」という要件があるため `openWithRetry()` を自作する。

---

## 5. システム構成（第一段階: 自己位置推定のみ）

```
config        パラメータを SSM property で全プロセスに配信。常駐するだけ
urg   UST-20LX を読み、urg_fs を配信
odom_conv     spur_odometry を地図座標へ剛体変換し、odom_gl を配信
ndt           urg_fs + estim_gl → PCL NDT2D でマッチング → ndt_gl を配信
localizer     odom_gl + ndt_gl → 相補フィルタで融合 → estim_gl を配信
```

ypspur-coordinator が別途起動しており、spur_odometry を約200Hz で供給する。

---

## 6. CMake の決定事項

### 方針
- **決め事は上流に1箇所**。C++標準・ビルドタイプ・出力先はルートでのみ指定する。
- ターゲット定義は各モジュールの CMakeLists.txt に置く（ルートには書かない）。
- `PUBLIC` / `PRIVATE` は「その依存が自分のヘッダに漏れているか」で判定する。
  - `utility.hpp` が `ssm.hpp` を include → `ssm` は PUBLIC
  - `param.cpp` の中だけで yaml-cpp を使う → yaml-cpp は PRIVATE

### find_package をどこに書くか（2026-09-16 決定）

**外部パッケージは、それを使うモジュールの CMakeLists.txt に書く。ルートには書かない。**

理由: PCL の find module は `add_definitions()` と `link_directories()` を呼ぶ。
これらはディレクトリスコープで効くため、ルートに書くと PCL と無関係な config や
utility にも PCL のコンパイル定義が付く。TC2025 のルートがこの状態だった。

現状の配置:
- `find_package( yaml-cpp REQUIRED )` → `config/CMakeLists.txt`
- `find_package( PCL 1.11 REQUIRED )` → `ndt/CMakeLists.txt`（ndt 実装時に追加）

ルートに書いた方がよくなるケース（将来の判断材料）:

| ケース | 理由 |
|---|---|
| 複数モジュールが同じパッケージを使う | 結果変数はサブディレクトリに継承されるので重複探索が省ける |
| バージョン要求を一元管理したい | モジュールごとに 1.10 / 1.11 と食い違うのを防げる |
| オプション依存で分岐したい | `find_package(X QUIET)` → `if(X_FOUND)` の判定を集約 |
| REQUIRED で早期に落としたい | 深いサブディレクトリまで進んでから失敗するのを避ける |

### 個別の決定

| 項目 | 決定 | 備考 |
|---|---|---|
| 出力先の指定 | `CMAKE_RUNTIME_OUTPUT_DIRECTORY` | TC2025 は旧仕様の `EXECUTABLE_OUTPUT_PATH`。現行仕様へ変更 |
| 警告オプション (`-Wall` 等) | **入れない** | TC2025 に無いため |
| C++標準 | ルートで `CMAKE_CXX_STANDARD 17` + `CMAKE_CXX_EXTENSIONS OFF` | `OFF` により `-std=c++14` が生成され、TC2025 の config と同じフラグになる |
| ビルドタイプ | `if( NOT CMAKE_BUILD_TYPE )` で Release | TC2025 は無条件代入で Debug ビルド不可だった。**TC2025 に無い挙動**。戻す選択肢あり |
| `message(STATUS ...)` のフラグ表示 | 省略 | TC2025 には有る。必要なら追加 |
| ビルド確認の方法 | `config.cpp` に `int main(){ return 0; }` だけ先に書く | 空ファイルだと main が無くリンクエラーになるため |
| `cmake_minimum_required` | `VERSION 3.17` | TC2025 は 3.15 |
| ディレクトリ構成 | `include/` (公開ヘッダ) + `src/` (実装と内部ヘッダ) | TC2025 は `src/` のみ。詳細は §3 |
| `utility` の外部依存 | **無し**（`target_link_libraries` の行を削除） | openWithRetry() を書かない間、utility は SSM を使わない。urg_handler 実装時に `PUBLIC ssm` を戻す |
| `target_include_directories` | utility は `PUBLIC include`、config は `PRIVATE include` | config は `src/` から `include/` を参照するため 1 行必要。`param.hpp` は `config.cpp` と同じ `src/` なので指定不要 |

### ディレクトリ構成

```
toyosu_challenge/
  CMakeLists.txt
  bin/                      実行ファイル出力先
  docs/decisions.md
  utility/
    CMakeLists.txt
    include/utility.hpp     公開
    src/utility.cpp
  config/
    CMakeLists.txt
    include/config.hpp      公開
    src/param.hpp           内部
    src/param.cpp
    src/config.cpp
    param/toyosu.yaml
```

`add_executable` は SSM のプロセス構成図に名前が載るものだけ（最終的に config /
urg_handler / odom_conv / ndt / localizer の 5 個）。`utility` は関数の置き場であり
プロセスではないので `add_library`。`config.hpp` は型定義のみでコンパイルすべき
実体が無いため、ライブラリにはできない（INTERFACE ライブラリ案は §8 の未決事項）。

静的ライブラリは `CMAKE_RUNTIME_OUTPUT_DIRECTORY` の対象外なので、
`bin/` ではなく `build/utility/libutility.a` に出力される。

### 依存関係

```
config (exe) ──> utility (static lib) ──> ssm
      └────────> yaml-cpp, m, pthread
```

yaml-cpp に依存するのは config だけ。他プロセスは config.hpp を include して
構造体を受け取るだけなので、yaml-cpp に依存しない。

---

## 7. config の設計

- パラメータの値は YAML に記述し、config プロセスだけが読む。
- config は `config_property` に全パラメータを詰め、`setProperty()` で配信する。
- 各プロセスは `getProperty()` で構造体全体を受け取り、自分に関係するメンバだけ参照する。
- `getProperty()` は起動時1回きり。パラメータを変更したらそのプロセスを再起動する。
- config は config ストリームのみを create する。他プロセスのストリームには触らない。

### config が常駐しなければならない理由
SSM は `release()` を呼ぶとストリームの実体が消える。config が終了すると全プロセスが
`getProperty()` に失敗するため、「property を設定して終了」はできない。

### サブ構造体の分割基準
**「どのプロセスが読むか」と1対1で対応させる。** 各構造体の直前に読み手プロセス名を
コメントで明記する。TC2025 は融合率 `ndt_alpfa` を `Control_info`（制御用）に入れるなど、
使用者と無関係な切り方をしていて可読性を損ねていた。

### デフォルト値を先に埋める理由
ROS2 の `declare_parameter(name, default_value)` と同じ役割。YAML にキーが無い、
あるいはキー名を書き間違えた場合でも、ゼロ埋めの値で走る事故を防ぐ。

**yaml-cpp はキー名の typo を検出できない。** 書き間違えたキーは「存在しない」として
扱われ、デフォルト値のまま静かに進む。これを検出する最後の砦が `printParam()` である。
起動時に print 出力と YAML の値が一致しているか目視すること。

---

## 8. 未決事項（勝手に実装しないこと）

### 構造体のフィールド
- `robot_param` の `width` / `length`: 第一段階では未使用。障害物検知で必要になる。
  今入れるか後で追加するか未決。
- `ndt_param` に `init_guess_source` を追加するか: NDT の初期推定を estim_gl にするか
  odom_gl にするかの切り替え。**現時点では追加しない。**
- `fusion_param` に `offset_mode` を追加するか: 補正量を代入形 (`offset = α × diff`) に
  するか累積形 (`offset += α × diff`) にするか。**現時点では追加しない。**
- `fusion_param.score_threshold`: 第一段階では未使用。定義だけ置く。

### config.hpp で確定した値（2026-09-16）
- `SNAME_CONFIG` = `"toyosu_config"`（仮ではなく確定）
- `CONFIG_VER` = `"2026.09.16"`
- 文字列長: `map_path[256]` / `wp_path[256]` / `device[64]` / `ver[64]`
- `robot_param` の `width` / `length` は**入れる**（TC2025 の `Robot_info` にも実績あり。
  第一段階では読まないが障害物検知で必要）
- `fusion_param.score_threshold` は定義だけ置き、実装では使わない
- TC2025 の `CONFIG_INFO` は入れない
- `config.hpp` は何も include しない（`_STRLEN` を使わないため utility にも依存しない）

### param.cpp で確定した実装方針（2026-09-16）
- 文字列コピーは `snprintf( dst, sizeof( dst ), "%s", src )` の 1 行（`strncpy` は切り詰め時に
  終端 NUL を付けないため使わない。TC2025 の `sprintf` は長さ制限が無いので使わない）
- 3 要素配列はヘルパを作らず各 loader に `for` を直書き
- `setDefault()` の冒頭で `memset` する（将来メンバを追加したときの保険。TC2025 には無い記述）
- `printParam()` の書式は `ラベル : 値 [単位]`（TC2025 の「値 → # → 説明」形式は .cfg の並びに
  対応させるためのもので、YAML では不要）
- デフォルト値は本番値とわざと変える（例: `urg.device` は `192.168.0.10`、YAML は `192.168.9.19`）。
  一致させるとキー名の typo に気づけなくなるため
- `param.cpp` は `utility.hpp` を include する（`_X` / `_Y` / `_YAW` を使うため）

### パラメータファイル
- 1ファイルか分割か: 現状 `toyosu.yaml` 1ファイル。
- パスを相対で持つか絶対に展開するか: 未決。現時点では YAML の文字列をそのまま保持する。
- バリデーション（`resolution <= 0` チェック、地図ファイルの存在確認など）: **実装しない。**
  `printParam()` の目視確認で代替。

### 実測値（すべて暫定値）
- `lidar_offset`: 車輪中心から LiDAR までの距離が未測定
- `start_pose`: Start地点の地図座標が未確定
- 車体寸法: 未測定

### utility の API
- `openWithRetry()`: **今回は実装しない。** 仕様書 §4 は要件に挙げているが、§9「utility.hpp は
  添字定数と trans_q() のみ」が実装順として優先。config は他ストリームを読まないため利用者が
  いない。**最初の利用者は urg_handler**（config を getProperty() で読むため）なので、
  そのときに追加する。シグネチャの結論（`SSMApiBase&` で受けられる）は §4 に記録済み。
- `transformPose()` / `transformPoint()`: **今回は実装しない。** 引数の向き
  （offset を「変換先から見た変換元」とするか逆か）は、odom_conv で実際に使ってみないと
  決めきれないため。先に API を決めると手戻りになる。

### その他
- ストリーム名 `SNAME_CONFIG` = `"toyosu_config"` は仮。変更可。
- config の `create()` 引数（保存秒数・周期）: data を使わないため実質任意。
- `config.hpp` を他プロセスへ渡す方法: `add_library(toyosu_config INTERFACE)` +
  `target_include_directories(... INTERFACE include)` でヘッダオンリーライブラリに
  する案があるが、採用するかは未決。判断は urg_handler の実装時。
  `include/` へ分割済みなので、必要になればそのまま書ける。
- 地図ファイル名: リポジトリの実体は `maps/toyosu_11f_map_all_2D.pcd` だが、
  仕様書では `maps/toyosu_11f_map_all_20cm_2D.pcd` となっている。**要確認。**

---

## 9. 実装順

```
[x] CMakeLists.txt 3ファイル（空ファイル + 暫定 main でビルド確認）
[x] utility/include/utility.hpp  添字定数 (_X/_Y/_YAW) と trans_q() の宣言のみ
[ ] utility/src/utility.cpp
[x] config/include/config.hpp    型定義
[x] config/src/param.hpp         公開インタフェース 2関数のみ
[x] config/src/param.cpp         setDefault() → 各 loader → printParam()
[ ] config/src/config.cpp        main
[ ] config/param/toyosu.yaml
[ ] ./bin/config を実行し、YAML の値が正しく print されることを確認
```

この時点で基盤は完成。以降 urg_handler → odom_conv → ndt → localizer の順に実装する。

---

## 10. 却下・撤回した提案の記録

同じ提案を繰り返さないための記録。

| 提案 | 却下理由 |
|---|---|
| `-Wall -Wextra` を付ける | TC2025 に無い |
| `gShutOff` を `volatile sig_atomic_t` にする | TC2025 は `bool`。config.cpp の実装時に改めて相談 |
| `copyStr()` / `loadArray3()` ヘルパの追加 | TC2025 に無い。param.cpp の実装時に改めて相談 |
| `openWithRetry()` の代わりに SSM 既存の `openWait()` を使う | 要件どおり自作する |
| ルートの `find_package(PCL)` をコメントアウト | 使うモジュール側に置く形で解決済み |
| utility.hpp に openWithRetry() を今書く | 仕様書 §9 が優先。利用者が現れる urg_handler 実装時に追加 |
| utility.hpp に TC2025 の `_STRLEN` / `_ROLL` / `_DEG2RAD()` 等を移植 | 第一段階で使わない。必要になった時点で移植する |
| utility.hpp に TC2025 の `isValidFile()` / `Gprint()` / `kbhit()` 等を移植 | 同上 |
| utility.hpp に `<cstdio>` / `<math.h>` を include | 宣言のみのファイルなので不要。`M_PI` を使う utility.cpp 側に置く |
