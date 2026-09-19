// パラメータの型定義

#ifndef CONFIG_HPP
#define CONFIG_HPP

#define SNAME_CONFIG "toyosu_config"
#define CONFIG_VER   "2026.09.16"

// ダミーのdata型
struct config_data {
  int dummy;
};

// 読み手: ndt
struct robot_param {
  double width;
  double length;
  double lidar_offset[ 3 ];
};

// 読み手: ndt, odom_conv
struct course_param {
  char map_path[ 256 ];
  double start_pose[ 3 ];
  char wp_path[ 256 ];
};

// 読み手: urg
struct urg_param {
  char device[ 64 ];
  int port;
  double range_min;
  double range_max;
};

// 読み手: ndt
struct ndt_param {
  double resolution;
  double step_size;
  double trans_epsilon;
  int max_iteration;
  double scan_leaf_size;
};

// 読み手: localizer
struct fusion_param {
  double alpha[ 3 ];
  double score_threshold;
};

// 最終的なパラメータの構造体
struct config_property {
  char ver[ 64 ];
  robot_param robot;
  course_param course;
  urg_param urg;
  ndt_param ndt;
  fusion_param fusion;
};

#endif // CONFIG_HPP


