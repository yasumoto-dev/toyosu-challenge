// UST-20LX との通信と、生データ (mm, 極座標) から urg_fs  (m, 直行座標) への変換

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include "lidar.hpp"

bool lidarHokuyo::initialize( const char *dev, double range_min, double range_max )
{
  this->range_min = range_min * 1000.0;  // [mm]
  this->range_max = range_max * 1000.0;  // [mm]
  
  // dev の文字列を崩さず、コピーを分解
  char buf_dev[ 256 ];
  snprintf( buf_dev, sizeof( buf_dev ), "%s", dev );

  // ethernet で接続
  bool flag_ethernet = false;
  if( strchr( buf_dev, ':' ) != NULL ){
    char *address = strtok( buf_dev, ":" );
    char *port_number = strtok( NULL, ":" );
    port = Scip2_OpenEthernet( address, atoi( port_number ) );
    if( port != NULL ) flag_ethernet = true;
  }
  if( !flag_ethernet ) port = Scip2_Open( dev, B0 );

  if( port == NULL ){
    fprintf( stderr, "ERROR: Failed to open device ( %s ).\n", dev );
    return false;
  }

  // パラメータ情報を取得
  if( !Scip2CMD_PP( port, &param ) ){
    fprintf( stderr, "ERROR: Failed to get sensor parameter ( PP ).\n" );
    Scip2_Close( port );
    port = NULL;
    return false;
  }
  // バージョン情報を取得
  if( !Scip2CMD_VV( port, &ver ) ){
    fprintf( stderr, "ERROR: Failed to get sensor version ( VV ).\n" );
    Scip2_Close( port );
    port = NULL;
    return false;
  }

  // バッファの初期化
  S2Sdd_Init( &buf );

  // マルチスキャンを開始
  if( !Scip2CMD_StartMS( port, param.step_min, param.step_max,
        1, 0, 0, &buf, SCIP2_ENC_3BYTE ) ){
    fprintf( stderr, "ERROR: Failed to start measurement ( MS ).\n" );
    S2Sdd_Dest( &buf );
    Scip2_Close( port );
    port = NULL;
    return false;
  }

  return true;
}

bool lidarHokuyo::getPointCloudData( urg_fs *urg )
{
  int ret = S2Sdd_Begin( &buf, &data );
  if( ret == -1 ){
    fprintf( stderr, "ERROR: Fatal error occurred.\n" );
    return false;
  }
  if( ret <= 0 ) return false;

  memset( urg->x, 0, sizeof( urg->x ) );
  memset( urg->y, 0, sizeof( urg->y ) );

  for( int i = 0; i < data->size && i < URG_DATA_MAX; i++ ){
    double dist = data->data[ i ];
    if( dist < param.dist_min || dist > param.dist_max ) continue; // センサ仕様外
    if( dist < range_min || dist > range_max ) continue;           // config による除去

	  double rad = ( double )( i - param.step_front + param.step_min ) * 2.0 * M_PI / param.step_resolution;
    urg->x[ i ] = dist * cos( rad ) * 0.001;  // [m]
    urg->y[ i ] = dist * sin( rad ) * 0.001;  // [m]
  }
  urg->size = ( data->size < URG_DATA_MAX ) ? data->size : URG_DATA_MAX;

  S2Sdd_End( &buf );

  return true;
}

void lidarHokuyo::getProperty( urg_property *urg )
{
  snprintf( urg->vender,   sizeof( urg->vender ),   "%s", ver.vender );
  snprintf( urg->product,  sizeof( urg->product ),  "%s", ver.product );
  snprintf( urg->firmware, sizeof( urg->firmware ), "%s", ver.firmware );
  snprintf( urg->protocol, sizeof( urg->protocol ), "%s", ver.protocol );
  snprintf( urg->serialno, sizeof( urg->serialno ), "%s", ver.serialno );
  snprintf( urg->model,    sizeof( urg->model ),    "%s", param.model );

  urg->dist_min        = param.dist_min * 0.001;        // [m]
  urg->dist_max        = param.dist_max * 0.001;        // [m]
  urg->step_resolution = param.step_resolution;
  urg->step_min        = param.step_min;
  urg->step_max        = param.step_max;
  urg->step_front      = param.step_front;
  urg->revolution      = param.revolution;

  printf( "\n<URG INFO>\n" );
  printf( "vender          : %s\n", urg->vender );
  printf( "product         : %s\n", urg->product );
  printf( "firmware        : %s\n", urg->firmware );
  printf( "protocol        : %s\n", urg->protocol );
  printf( "serial no       : %s\n", urg->serialno );
  printf( "model           : %s\n", urg->model );
  printf( "dist_min        : %.3f [m]\n", urg->dist_min );
  printf( "dist_max        : %.3f [m]\n", urg->dist_max );
  printf( "step_resolution : %d\n", urg->step_resolution );
  printf( "step_min        : %d\n", urg->step_min );
  printf( "step_max        : %d\n", urg->step_max );
  printf( "step_front      : %d\n", urg->step_front );
  printf( "revolution      : %d [rpm]\n\n", urg->revolution );
}

void lidarHokuyo::terminate( void )
{
  if( port == NULL ) return;

  Scip2CMD_StopMS( port, &buf );     // 受信スレッド停止, 計測停止 ( QT )
  S2Sdd_Dest( &buf );                // mutex 破棄, バッファ解放
  Scip2_Close( port );
  port = NULL;
}
