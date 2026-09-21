// urg_fs ストリームの型定義

#ifndef URG_HPP
#define URG_HPP

#define SNAME_URG "urg_fs"
#define URG_DATA_MAX 1081        // UST-20LX: step 0 ~ 1080
#define URG_STR_LEN 128          // SCIP2_MAX_LENGTH と同値

struct urg_fs {
  unsigned int size;             // ステップ数
  double x[ URG_DATA_MAX ];      // [m]
  double y[ URG_DATA_MAX ];      // [m]
};

// 起動時に PP / VV コマンドから取得するセンサ固有情報
struct urg_property {
  char vender[ URG_STR_LEN ];
  char product[ URG_STR_LEN ];
  char firmware[ URG_STR_LEN ];
  char protocol[ URG_STR_LEN ];
  char serialno[ URG_STR_LEN ];
  char model[ URG_STR_LEN ];
  double dist_min;               // [m] 計測可能な最小距離
  double dist_max;               // [m] 計測可能な最大距離
  int step_resolution;           // 1回転あたりのステップ数
  int step_min;
  int step_max;
  int step_front;                // 正面方向のスッテプ番号
  int revolution;                // [rpm] スキャン回転数
};

#endif // URG_HPP
