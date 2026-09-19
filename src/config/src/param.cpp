// yamlからパラメータの読み込み・デフォルト値・表示

#include <cstdio>
#include <cstring>
#include <string>

#include <yaml-cpp/yaml.h>

#include "utility.hpp"
#include "param.hpp"

static void setDefault( config_property *conf );
static void loadRobot ( const YAML::Node &n, robot_param  *p );
static void loadCourse( const YAML::Node &n, course_param *p );
static void loadUrg   ( const YAML::Node &n, urg_param    *p );
static void loadNdt   ( const YAML::Node &n, ndt_param    *p );
static void loadFusion( const YAML::Node &n, fusion_param *p );

// デフォルト値の設定
static void setDefault( config_property *conf )
{
  memset( conf, 0, sizeof( config_property ) );

  snprintf( conf->ver, sizeof( conf->ver ), "%s", CONFIG_VER );

  conf->robot.width  = 0.40;
  conf->robot.length = 0.50;
  conf->robot.lidar_offset[ _X ]   = 0.0;
  conf->robot.lidar_offset[ _Y ]   = 0.0;
  conf->robot.lidar_offset[ _YAW ] = 0.0;

  snprintf( conf->course.map_path, sizeof( conf->course.map_path ), "%s", "" );
  snprintf( conf->course.wp_path,  sizeof( conf->course.wp_path ), "%s", "" );
  conf->course.start_pose[ _X ]   = 0.0;
  conf->course.start_pose[ _Y ]   = 0.0;
  conf->course.start_pose[ _YAW ] = 0.0;

  snprintf( conf->urg.device, sizeof( conf->urg.device ), "%s","192.168.0.10" );
  conf->urg.port      = 10940;
  conf->urg.range_min = 0.10;
  conf->urg.range_max = 20.0;

  conf->ndt.resolution     = 0.1;
  conf->ndt.step_size      = 0.1;
  conf->ndt.trans_epsilon  = 0.01;
  conf->ndt.max_iteration  = 30;
  conf->ndt.scan_leaf_size = 0.05;

  conf->fusion.alpha[ _X ]       = 0.5;
  conf->fusion.alpha[ _Y ]       = 0.5;
  conf->fusion.alpha[ _YAW ]     = 0.3;
  conf->fusion.score_threshold   = 2.0;
}

// robot_param
static void loadRobot( const YAML::Node &n, robot_param *p )
{
  if( n[ "width"  ] ) p->width  = n[ "width"  ].as< double >();
  if( n[ "length" ] ) p->length = n[ "length" ].as< double >();

  if( n[ "lidar_offset" ] ){
    for(int i = 0; i < 3; i++ ){
      p->lidar_offset[ i ] = n[ "lidar_offset" ][ i ].as< double >();
    }
  }
}

// course_param
static void loadCourse( const YAML::Node &n, course_param *p )
{
  if( n[ "map_path" ] ){
    snprintf( p->map_path, sizeof( p->map_path ), "%s", n[ "map_path" ].as< std::string >().c_str() );
  }
  if( n[ "wp_path" ] ){
    snprintf( p->wp_path, sizeof( p->wp_path ), "%s", n[ "wp_path" ].as< std::string >().c_str() );
  }
  if( n[ "start_pose" ] ){
    for( int i = 0; i < 3; i++ ){
      p->start_pose[ i ] = n[ "start_pose" ][ i ].as< double >();
    }
  } 
}

// urg_param
static void loadUrg( const YAML::Node &n, urg_param *p )
{
  if( n[ "device" ] ){
   snprintf( p->device, sizeof( p->device ), "%s", n[ "device" ].as< std::string >().c_str() ); 
  }
  if( n[ "port"      ] ) p->port      = n[ "port"      ].as< int    >();
  if( n[ "range_min" ] ) p->range_min = n[ "range_min" ].as< double >();
  if( n[ "range_max" ] ) p->range_max = n[ "range_max" ].as< double >();
}

// ndt_param
static void loadNdt( const YAML::Node &n, ndt_param *p )
{
  if( n[ "resolution"     ] ) p->resolution     = n[ "resolution"     ].as< double >();
  if( n[ "step_size"      ] ) p->step_size      = n[ "step_size"      ].as< double >();
  if( n[ "trans_epsilon"  ] ) p->trans_epsilon  = n[ "trans_epsilon"  ].as< double >();
  if( n[ "max_iteration"  ] ) p->max_iteration  = n[ "max_iteration"  ].as< int    >();
  if( n[ "scan_leaf_size" ] ) p->scan_leaf_size = n[ "scan_leaf_size" ].as< double >();
}

// fusion_param
static void loadFusion( const YAML::Node &n, fusion_param *p )
{
  if( n[ "alpha" ] ){
    for( int i = 0; i < 3; i++ ){
      p->alpha[ i ] = n[ "alpha" ][ i ].as< double >();
    }
  }
  if( n[ "score_threshold" ] ) p->score_threshold = n[ "score_threshold" ].as< double >();
}

// パラメータをロードするメイン処理
bool loadParam( const char *path, config_property *conf )
{
  setDefault( conf );

  try{
    YAML::Node root = YAML::LoadFile( path );

    if( root[ "robot"  ] ) loadRobot (root[ "robot"  ], &conf->robot  );
    if( root[ "course" ] ) loadCourse(root[ "course" ], &conf->course );
    if( root[ "urg"    ] ) loadUrg   (root[ "urg"    ], &conf->urg    );
    if( root[ "ndt"    ] ) loadNdt   (root[ "ndt"    ], &conf->ndt    );
    if( root[ "fusion" ] ) loadFusion(root[ "fusion" ], &conf->fusion );
  }
  catch( const YAML::Exception &e ){
    fprintf( stderr, "ERROR : cannot load '%s'\n", path );
    fprintf( stderr, "        %s\n", e.what() );
    return false;
  }

  snprintf( conf->ver, sizeof( conf->ver ), "%s", CONFIG_VER );

  return true;
}

// パラメータの表示
void printParam( const config_property *conf )
{
  printf( "\n" );
  printf( "===== toyosu_challenge config =====\n" );
  printf( "  ver              : %s\n", conf->ver );

  printf( "\n[ robot ] -> ndt\n" );
  printf( "  width            : %8.3f [m]\n", conf->robot.width  );
  printf( "  length           : %8.3f [m]\n", conf->robot.length );
  printf( "  lidar_offset     : %8.3f [m] %8.3f [m] %8.3f [rad]\n",
                  conf->robot.lidar_offset[ _X ],
                  conf->robot.lidar_offset[ _Y ],
                  conf->robot.lidar_offset[ _YAW ] );

  printf( "\n[ course ] -> ndt, odom_conv\n" );
  printf( "  map_path         : %s\n", conf->course.map_path );
  printf( "  wp_path          : %s\n", conf->course.wp_path  );
  printf( "  start_pose       : %8.3f [m] %8.3f [m] %8.3f [rad]\n",
                  conf->course.start_pose[ _X ],
                  conf->course.start_pose[ _Y ],
                  conf->course.start_pose[ _YAW ] );

  printf( "\n[ urg ] -> urg\n" );
  printf( "  device           : %s\n",        conf->urg.device    );
  printf( "  port             : %8d\n",       conf->urg.port      );
  printf( "  range_min        : %8.3f [m]\n", conf->urg.range_min );
  printf( "  range_max        : %8.3f [m]\n", conf->urg.range_max );

  printf( "\n[ ndt ] -> ndt\n" );
  printf( "  resolution       : %8.3f [m]\n", conf->ndt.resolution     );
  printf( "  step_size        : %8.3f\n",     conf->ndt.step_size      );
  printf( "  trans_epsilon    : %8.4f\n",     conf->ndt.trans_epsilon  );
  printf( "  max_iteration    : %8d\n",       conf->ndt.max_iteration  );
  printf( "  scan_leaf_size   : %8.3f [m]\n", conf->ndt.scan_leaf_size );

  printf( "\n[ fusion ] -> localizer\n" );
  printf( "  alpha            : %8.3f %8.3f %8.3f ( x, y, yaw )\n",
                  conf->fusion.alpha[ _X ],
                  conf->fusion.alpha[ _Y ],
                  conf->fusion.alpha[ _YAW ] );
  printf( "  score_threshold  : %8.3f\n", conf->fusion.score_threshold );

  printf( "\n===================================\n\n" );
}
