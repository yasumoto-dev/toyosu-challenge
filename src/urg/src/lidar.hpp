// UST-20LX のセンサドライバ

#ifndef LIDAR_HPP
#define LIDAR_HPP

#include <scip2awd.h>

#include "urg.hpp"

class lidarHokuyo {
private:
  S2Port *port;                // センサとの接続
  S2Sdd_t buf;                 // 受信スレッドが書き込む 3 面バッファ
  S2Scan_t *data;              // バッファへのポインタ 
  S2Param_t param;             // PPコマンド結果 
  S2Ver_t ver;                 // VV コマンド結果
  double range_min;
  double range_max;

public:
  lidarHokuyo( void ) : port( nullptr ), data( nullptr ),
                        range_min( 0.0 ), range_max( 0.0 ) { }
  ~lidarHokuyo( void ) { }

  bool initialize( const char *dev, double range_min, double range_max );
  bool getPointCloudData( urg_fs *urg );
  void getProperty( urg_property *urg );
  void terminate( void );
};

#endif // LIDAR_HPP
