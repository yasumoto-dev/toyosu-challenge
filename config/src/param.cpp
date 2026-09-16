// パラメータの読み込み・デフォルト値・表示

#include <cstdio>
#include <cstring>
#include <string>

#include <yaml-cpp/yaml.h>

#include "utility.hpp"
#include "param.hpp"

static void setDefault( config_property *cnf );
static void loadRobot ( const YAML::Node &n, robot_param  *p );
static void loadCourse( const YAML::Node &n, course_param *p );
static void loadUrg   ( const YAML::Node &n, urg_param    *p );
static void loadNdt   ( const YAML::Node &n, ndt_param    *p );
static void loadFusion( const YAML::Node &n, fusion_param *p );

// デフォルト値の設定
static void setDefault( config_property *cnf )
{
  memset(cnf, 0, sizeof(config_property));

  snprintf( cnf->ver, sizeof( cnf->ver ), "%s", CONFIG_VER );

  cnf->robot.width  = 0.45;
  cnf->robot.length = 0.60;
  cnf->robot.lidar_offset[ _X ]   = 0.0;
  cnf->robot.lidar_offset[ _Y ]   = 0.0;
  cnf->robot.lidar_offset[ _YAW ] = 0.0;

  snprintf( cnf->course.map_path, sizeof( cnf->course.map_path ), "%s", "" );
  snprintf( cnf->course.wp_path,  sizeof( cnf->course.wp_path ), "%s", "" );
  cnf->course.start_pose[ _X ]   = 0.0;
  cnf->course.start_pose[ _Y ]   = 0.0;
  cnf->course.start_pose[ _YAW ] = 0.0;

  snprintf( cnf->urg.device, sizeof( cnf->urg.device ), "%s","192.168.0.10" );
  cnf->urg.port      = 10940;
  cnf->urg.range_min = 0.10;
  cnf->urg.range_max = 20.0;

  cnf->ndt.resolution     = 0.1;
  cnf->ndt.step_size      = 0.1;
  cnf->ndt.trans_epsilon  = 0.01;
  cnf->ndt.max_iteration      = 30;
  cnf->ndt.scan_leaf_size = 0.05;

  cnf->fusion.alpha[ _X ]       = 0.5;
  cnf->fusion.alpha[ _Y ]       = 0.5;
  cnf->fusion.alpha[ _YAW ]     = 0.3;
  cnf->fusion.score_threshold   = 2.0;
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
bool loadParam( const char *path, config_property *cnf )
{
  setDefault( cnf );

  try{
    YAML::Node root = YAML::LoadFile( path );

    if( root[ "robot"  ] ) loadRobot (root[ "robot"  ], &cnf->robot  );
    if( root[ "course" ] ) loadCourse(root[ "course" ], &cnf->course );
    if( root[ "urg"    ] ) loadUrg   (root[ "urg"    ], &cnf->urg    );
    if( root[ "ndt"    ] ) loadNdt   (root[ "ndt"    ], &cnf->ndt    );
    if( root[ "fusion" ] ) loadFusion(root[ "fusion" ], &cnf->fusion );
  }
  catch( const YAML::Exception &e ){
    fprintf( stderr, "ERROR : cannot load '%s'\n", path );
    fprintf( stderr, "        %s\n", e.what() );
    return false;
  }

  snprintf( cnf->ver, sizeof( cnf->ver ), "%s", CONFIG_VER );

  return true;
}

// パラメータの表示
void printParam( const config_property *cnf )
{
    printf( "\n" );
    printf( "===== toyosu_challenge config =====\n" );
    printf( "  ver              : %s\n", cnf->ver );

    printf( "\n[ robot ] -> ndt\n" );
    printf( "  width            : %8.3f [m]\n", cnf->robot.width  );
    printf( "  length           : %8.3f [m]\n", cnf->robot.length );
    printf( "  lidar_offset     : %8.3f [m] %8.3f [m] %8.3f [rad]\n",
                    cnf->robot.lidar_offset[ _X ],
                    cnf->robot.lidar_offset[ _Y ],
                    cnf->robot.lidar_offset[ _YAW ] );

    printf( "\n[ course ] -> ndt, odom_conv\n" );
    printf( "  map_path         : %s\n", cnf->course.map_path );
    printf( "  wp_path          : %s\n", cnf->course.wp_path  );
    printf( "  start_pose       : %8.3f [m] %8.3f [m] %8.3f [rad]\n",
                    cnf->course.start_pose[ _X ],
                    cnf->course.start_pose[ _Y ],
                    cnf->course.start_pose[ _YAW ] );

    printf( "\n[ urg ] -> urg_handler\n" );
    printf( "  device           : %s\n",        cnf->urg.device    );
    printf( "  port             : %8d\n",       cnf->urg.port      );
    printf( "  range_min        : %8.3f [m]\n", cnf->urg.range_min );
    printf( "  range_max        : %8.3f [m]\n", cnf->urg.range_max );

    printf( "\n[ ndt ] -> ndt\n" );
    printf( "  resolution       : %8.3f [m]\n", cnf->ndt.resolution     );
    printf( "  step_size        : %8.3f\n",     cnf->ndt.step_size      );
    printf( "  trans_epsilot    : %8.4f\n",     cnf->ndt.trans_epsilon  );
    printf( "  max_iteration    : %8d\n",       cnf->ndt.max_iteration  );
    printf( "  scan_leaf_size   : %8.3f [m]\n", cnf->ndt.scan_leaf_size );

    printf( "\n[ fusion ] -> localizer\n" );
    printf( "  alpha            : %8.3f %8.3f %8.3f ( x, y, yaw )\n",
                    cnf->fusion.alpha[ _X ],
                    cnf->fusion.alpha[ _Y ],
                    cnf->fusion.alpha[ _YAW ] );
    printf( "  score_threshold  : %8.3f\n", cnf->fusion.score_threshold );

    printf( "\n===================================\n\n" );
}
