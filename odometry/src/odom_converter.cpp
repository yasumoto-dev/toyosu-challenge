// spur_odometry を地図座標系に剛体変換し、odom_glを配信

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <stdexcept>

#include <getopt.h>
#include <signal.h>

#include <ssm.hpp>
#include <ssmtype/spur-odometry.h>

#include "utility.hpp"
#include "config.hpp"
#include "odom_gl.hpp"

static int gShutOff = 0;
static unsigned int dT = 5; // [ms]

static double gStart[ 3 ];  // 地図座標系のスタート姿勢 ( x[m], y[m], yaw[rad] )
static Spur_Odometry gInit; // 起動時の spur_odometry ( Yp-Spur 座標系 )
static double gCos, gSin;   // 両座標系の回転差 th の cos / sin

static SSMApi< config_data, config_property > *CONF;
static SSMApi< Spur_Odometry > *SPUR;
static SSMApi< odom_gl > *ODM;

static void ctrlC( int aStatus );
static void setSigInt( void );
static void printShortHelp( const char *programName );
static bool setOption( int aArgc, char *aArgv[ ] );
static void setupSSM( void );
static void Terminate( void );
static void initTransform( const course_param *course, const Spur_Odometry *init );
static void transformToMap( const Spur_Odometry *src, odom_gl *dst );


int main( int aArgc, char *aArgv[ ] )
{
  if( !setOption( aArgc, aArgv ) ) return EXIT_FAILURE;

  SSMApi< config_data, config_property > conf( SNAME_CONFIG, 0 );
    CONF = &conf;
  SSMApi< Spur_Odometry > spur( SNAME_ODOMETRY, 0 );
    SPUR = &spur;
  SSMApi< odom_gl > odm( SNAME_ODOM, 0 );
    ODM = &odm;

  try {
    setupSSM( );
    setSigInt( );

    fprintf( stderr, "waiting for spur_odometry ...\n" );
    while( !gShutOff && !spur.readLast( ) ){
      usleepSSM( 100 * 1000 );
    }

    initTransform( &conf.property.course, &spur.data );

    while( !gShutOff ){
      if( spur.readNew( ) ){
        transformToMap( &spur.data, &odm.data );
        odm.write( spur.time );
      } else {
        usleepSSM( dT * 1000 );
      }
    }
  }
  catch( const std::runtime_error &error ){
    fprintf( stderr, "%s\n", error.what( ) );
  }
  catch( ... ){
    fprintf( stderr, "An unknown fatal error has occured. Aborting.\n" );
  }

  Terminate( );

  return EXIT_SUCCESS;
}


static void setupSSM( void )
{
  fprintf( stderr, "initializing ssm ... " );
  if( !initSSM( ) ){
    throw std::runtime_error( "ERROR : fail to initialize ssm." );
  }
  fprintf( stderr, "OK.\n" );

  fprintf( stderr, "open   toyosu_config ... " );
  if( !CONF->open( SSM_READ ) ){
    throw std::runtime_error( "ERROR : fail to open toyosu_config on ssm." );
  }
  if( !CONF->getProperty( ) ){
    throw std::runtime_error( "ERROR : fail to get toyosu_config.property." );
  }
  fprintf( stderr, "OK.\n" );

  fprintf( stderr, "open   spur_odometry ... " );
  if( !SPUR->open( SSM_READ ) ){
    throw std::runtime_error( "ERROR : fail to open spur_odometry on ssm." );
  }
  fprintf( stderr, "OK.\n" );

  fprintf( stderr, "create odom_gl ... " );
  if( !ODM->create( 1.0, ( double )dT/1000.0 ) ){
    throw std::runtime_error( "ERROR : fail to create odom_gl on ssm." );
  }
  fprintf( stderr, "OK.\n" );
}

static void Terminate( void )
{
  CONF->release( );
  SPUR->release( );
  ODM->release( );
  endSSM( );
  printf( "\nend\n" );
}

// 起動時の姿勢から両座標系の関係を確定 ( course: 地図座標系, init: Yp-Spur座標系 )
static void initTransform( const course_param *course, const Spur_Odometry *init )
{
  for( int i = 0; i < 3; i++ ){
    gStart[ i ] = course->start_pose[ i ];
  }
  gInit = *init;

  double th = trans_q( gStart[ _YAW ] - gInit.theta );
  gCos = cos( th );
  gSin = sin( th );

  printf( "start_pose : %7.3f [m] %7.3f [m] %7.3f [rad] (%6.1f [deg])\n",
          gStart[ _X ], gStart[ _Y ], gStart[ _YAW ], gStart[ _YAW ] * 180.0 / M_PI );
  printf( "spur init  : %7.3f [m] %7.3f [m] %7.3f [rad] (%6.1f [deg])\n",
          gInit.x, gInit.y, gInit.theta, gInit.theta * 180.0 / M_PI );
  printf( "ratation   : %7.3f [rad] (%6.1f [deg])\n", th, th * 180.0 /M_PI );  
}

// Yp-Spur座標系の姿勢を地図座標系へ変換 ( ロボット位置の座標変換 )
static void transformToMap( const Spur_Odometry *src, odom_gl *dst )
{
  double dx = src->x - gInit.x; //Yp-Spur座標系での移動量
  double dy = src->y - gInit.y;

  dst->pose[ _X ]   = gStart[ _X ] + gCos * dx - gSin * dy; // odom_gl の位置 ( thから回転行列を求める )
  dst->pose[ _Y ]   = gStart[ _Y ] + gSin * dx + gCos * dy;
  dst->pose[ _YAW ] = trans_q( gStart[ _YAW ] + ( src->theta - gInit.theta ) );

  dst->v = src->v;
  dst->w = src->w;
}

static void ctrlC( int aStatus )
{
  signal( SIGINT, SIG_DFL );
  gShutOff = 1;
}

static void setSigInt( void )
{
  struct sigaction sig;
  memset( &sig, 0, sizeof( sig ) );
  sig.sa_handler = ctrlC;
  sigaction( SIGINT, &sig, NULL );
}

static void printShortHelp( const char *programName )
{
  fprintf( stderr, "HELP\n" );
  fprintf( stderr, "\t$ %s\n", programName );
  fprintf( stderr, "OPTION\n" );
  fprintf( stderr, "\t-h | --help : Show this help.\n" );
  fprintf( stderr, "\n" );
}

static bool setOption( int aArgc, char *aArgv[ ] )
{
  int opt, optIndex = 0;
  struct option longOpt[ ] = {
    { "help", 0, 0, 'h' },
    { 0, 0, 0, 0 }
  };

  while( ( opt = getopt_long( aArgc, aArgv, "h", longOpt, &optIndex ) ) != -1 ){
    switch( opt ){
    case 'h':
      printShortHelp( aArgv[ 0 ] );
      return false;
    default:
      fprintf( stderr, "help : %s -h\n", aArgv[ 0 ] );
      return false;
    }
  }

  return true;
}
