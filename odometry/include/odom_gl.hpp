// 地図座標系のオドメトリ姿勢の型定義

#ifndef ODOM_GL_HPP
#define ODOM_GL_HPP

#define SNAME_ODOM "odom_gl"

struct odom_gl{
  double pose[ 3 ]; // 地図座標系・車輪中心 ( x[m], y[m], yaw[rad])
  double v;         // 並進速度 [m/s] (後退は負)
  double w;         // 角速度 [rad/s]
};

#endif // ODOM_GL_HPP
