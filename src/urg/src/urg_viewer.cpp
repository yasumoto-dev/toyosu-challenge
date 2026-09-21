// urg_fs を購読して点群を描画する確認用ツール

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <stdexcept>

#include <getopt.h>
#include <signal.h>

#include <ssm.hpp>
#include <opencv2/opencv.hpp>

#include "urg.hpp"

static int gShutOff = 0;
static unsigned int dT = 40;              // [ms]

static bool gFlagLaser = false;

static const int W = 600, H = 600;        // [pixel] 描画領域
static const double SCALE = 50.0;         // [pixel/m]
static const int GRID = 6;                // [m] グリッドの範囲

static SSMApi< urg_fs > *URG;

static void ctrlC( int aStatus );
static void setSigInt( void );
static void printShortHelp( const char *programName );
static bool setOption( int aArgc, char *aArgv[ ] );
static void setupSSM( void );
static void Terminate( void );
static cv::Point toPixel( double x, double y );
static void drawGrid( cv::Mat &img );
static void drawScan( cv::Mat &img, const urg_fs *urg );


int main( int aArgc, char *aArgv[ ] )
{
  if( !setOption( aArgc, aArgv ) ) return EXIT_FAILURE;

  SSMApi< urg_fs > urg( SNAME_URG, 0 );
    URG = &urg;

  try {
    setupSSM( );
    setSigInt( );

    cv::namedWindow( "urg_viewer", cv::WINDOW_NORMAL );
    cv::resizeWindow( "urg_viewer", W, H );
    cv::Mat img( H, W, CV_8UC3 );
    img = cv::Scalar( 255, 255, 255 );

    printf( "start urg_viewer\n" );
    while( !gShutOff ){
      if( urg.readNew( ) ){
        img = cv::Scalar( 255, 255, 255 );
        drawGrid( img );
        drawScan( img, &urg.data );
      }
      cv::imshow( "urg_viewer", img );
      cv::waitKey( 1 );
      usleepSSM( dT * 1000 );
    }

    cv::destroyAllWindows( );
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

  fprintf( stderr, "open   urg_fs ... " );
  if( !URG->open( SSM_READ ) ){
    throw std::runtime_error( "ERROR : fail to open urg_fs on ssm." );
  }
  fprintf( stderr, "OK.\n" );
}

static void Terminate( void )
{
  URG->release( );
  endSSM( );
  printf( "\nend\n" );
}

// LiDAR 座標系 [m] を画像のピクセル座標へ変換する
static cv::Point toPixel( double x, double y )
{
  int px = W / 2 + ( int )( x * SCALE );
  int py = H / 2 - ( int )( y * SCALE );

  return cv::Point( px, py );
}

static void drawGrid( cv::Mat &img )
{
  for( int m = -GRID; m <= GRID; m++ ){
    cv::line( img, toPixel( m, -GRID ), toPixel( m, GRID ), cv::Scalar( 220, 220, 220 ), 1 );
    cv::line( img, toPixel( -GRID, m ), toPixel( GRID, m ), cv::Scalar( 220, 220, 220 ), 1 );
  }
  cv::line( img, toPixel( -GRID, 0 ), toPixel( GRID, 0 ), cv::Scalar( 100, 100, 100 ), 1 );
  cv::line( img, toPixel( 0, -GRID ), toPixel( 0, GRID ), cv::Scalar( 100, 100, 100 ), 1 );

  for( int m = -GRID; m <= GRID; m += 2 ){
    if( m == 0 ) continue;

    cv::Point px_label = toPixel( m, 0 );
    cv::putText( img, std::to_string( m ), cv::Point( px_label.x - 8, px_label.y + 18 ),
                 cv::FONT_HERSHEY_SIMPLEX, 0.42, cv::Scalar( 100, 100, 100 ), 1, cv::LINE_AA );

    cv::Point py_label = toPixel( 0, m );
    cv::putText( img, std::to_string( m ), cv::Point( py_label.x + 6, py_label.y + 18 ),
                 cv::FONT_HERSHEY_SIMPLEX, 0.42, cv::Scalar( 100, 100, 100 ), 1, cv::LINE_AA );
  }

  cv::Point origin = toPixel( 0, 0 );
  cv::putText( img, "0", cv::Point( origin.x + 6, origin.y + 18 ),
               cv::FONT_HERSHEY_SIMPLEX, 0.42, cv::Scalar( 100, 100, 100 ), 1, cv::LINE_AA );
}

// 無効点 ( x = y = 0 ) は描かない
static void drawScan( cv::Mat &img, const urg_fs *urg )
{
  for( unsigned int i = 0; i < urg->size && i < URG_DATA_MAX; i++ ){
    if( urg->x[ i ] == 0.0 && urg->y[ i ] == 0.0 ) continue;

    cv::Point p = toPixel( urg->x[ i ], urg->y[ i ] );
    if( gFlagLaser ) cv::line( img, toPixel( 0, 0 ), p, cv::Scalar( 0, 200, 0 ), 1 );
    cv::circle( img, p, 1, cv::Scalar( 0, 0, 255 ), -1 );
  }
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
  fprintf( stderr, "\t$ %s -l -t 40\n", programName );
  fprintf( stderr, "OPTION\n" );
  fprintf( stderr, "\t-l | --laser       : Draw laser.\n" );
  fprintf( stderr, "\t-t | --s_time TIME : Wait time. ( default : %d ms )\n", dT );
  fprintf( stderr, "\t-h | --help        : Show this help.\n" );
  fprintf( stderr, "\n" );
}

static bool setOption( int aArgc, char *aArgv[ ] )
{
  int opt, optIndex = 0;
  struct option longOpt[ ] = {
    { "laser",  0, 0, 'l' },
    { "s_time", 1, 0, 't' },
    { "help",   0, 0, 'h' },
    { 0, 0, 0, 0 }
  };

  while( ( opt = getopt_long( aArgc, aArgv, "lt:h", longOpt, &optIndex ) ) != -1 ){
    switch( opt ){
    case 'l':
      gFlagLaser = true;
      break;
    case 't':
      dT = atoi( optarg );
      break;
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
