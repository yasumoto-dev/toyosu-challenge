// urg_fs を購読して点群を描画する確認用ツール

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <stdexcept>
#include <string>

#include <getopt.h>
#include <signal.h>

#include <ssm.hpp>
#include <opencv2/opencv.hpp>

#include "urg.hpp"

static int gShutOff = 0;
static unsigned int dT = 40;              // [ms]

static bool gFlagLaser = false;

static const int GRID = 6;                // [m] グリッドの範囲
static const double SCALE = 50.0;         // [pixel/m]

static const int PLOT_W = ( int )( GRID * 2 * SCALE );   // 600 px
static const int PLOT_H = ( int )( GRID * 2 * SCALE );   // 600 px

static const int MARGIN_L = 78, MARGIN_R = 30, MARGIN_T = 56, MARGIN_B = 66;

static const int CANVAS_W = MARGIN_L + PLOT_W + MARGIN_R;
static const int CANVAS_H = MARGIN_T + PLOT_H + MARGIN_B;

static SSMApi< urg_fs > *URG;

static void ctrlC( int aStatus );
static void setSigInt( void );
static void printShortHelp( const char *programName );
static bool setOption( int aArgc, char *aArgv[ ] );
static void setupSSM( void );
static void Terminate( void );
static cv::Point toPixel( double x, double y );
static void drawGrid( cv::Mat &plot );
static void drawScan( cv::Mat &plot, const urg_fs *urg );
static void drawFrame( cv::Mat &img );


int main( int aArgc, char *aArgv[ ] )
{
  if( !setOption( aArgc, aArgv ) ) return EXIT_FAILURE;

  SSMApi< urg_fs > urg( SNAME_URG, 0 );
    URG = &urg;

  try {
    setupSSM( );
    setSigInt( );

    cv::namedWindow( "urg_viewer", cv::WINDOW_AUTOSIZE );
    cv::Mat img( CANVAS_H, CANVAS_W, CV_8UC3 );
    img = cv::Scalar( 255, 255, 255 );
    cv::Mat plot = img( cv::Rect( MARGIN_L, MARGIN_T, PLOT_W, PLOT_H ) );
    drawFrame( img );

    printf( "start urg_viewer\n" );
    while( !gShutOff ){
      if( urg.readNew( ) ){
        plot = cv::Scalar( 255, 255, 255 );
        drawGrid( plot );
        drawScan( plot, &urg.data );
      }
      cv::imshow( "urg_viewer", img );
      cv::waitKey( 1 );

      if( cv::getWindowProperty( "urg_viewer", cv::WND_PROP_VISIBLE ) < 1 ) break;

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

// LiDAR 座標系 [m] をプロット領域基準のピクセル座標へ変換する
static cv::Point toPixel( double x, double y )
{
  int px = PLOT_W / 2 + ( int )( x * SCALE );
  int py = PLOT_H / 2 - ( int )( y * SCALE );

  return cv::Point( px, py );
}

static void drawGrid( cv::Mat &plot )
{
  for( int m = -GRID; m <= GRID; m++ ){
    cv::line( plot, toPixel( m, -GRID ), toPixel( m, GRID ), cv::Scalar( 220, 220, 220 ), 1 );
    cv::line( plot, toPixel( -GRID, m ), toPixel( GRID, m ), cv::Scalar( 220, 220, 220 ), 1 );
  }
  cv::line( plot, toPixel( -GRID, 0 ), toPixel( GRID, 0 ), cv::Scalar( 180, 180, 180 ), 1 );
  cv::line( plot, toPixel( 0, -GRID ), toPixel( 0, GRID ), cv::Scalar( 180, 180, 180 ), 1 );
}

// 無効点 ( x = y = 0 ) は描かない
static void drawScan( cv::Mat &plot, const urg_fs *urg )
{
  for( unsigned int i = 0; i < urg->size && i < URG_DATA_MAX; i++ ){
    if( urg->x[ i ] == 0.0 && urg->y[ i ] == 0.0 ) continue;

    cv::Point p = toPixel( urg->x[ i ], urg->y[ i ] );
    if( gFlagLaser ) cv::line( plot, toPixel( 0, 0 ), p, cv::Scalar( 0, 200, 0 ), 1 );
    cv::circle( plot, p, 1, cv::Scalar( 0, 0, 255 ), -1 );
  }
}

// 枠線・目盛り・軸ラベル・タイトルを描く（プロット領域の外側のみ）
static void drawFrame( cv::Mat &img )
{
  cv::rectangle( img, cv::Rect( MARGIN_L - 1, MARGIN_T - 1, PLOT_W + 2, PLOT_H + 2 ), cv::Scalar( 0, 0, 0 ), 1 );

  static const int TICK_LEN = 5;

  for( int m = -GRID; m <= GRID; m++ ){
    cv::Point p = toPixel( m, 0 );
    int px = MARGIN_L + p.x;

    cv::line( img, cv::Point( px, MARGIN_T + PLOT_H ), cv::Point( px, MARGIN_T + PLOT_H + TICK_LEN ),
               cv::Scalar( 0, 0, 0 ), 1 );

    std::string label = std::to_string( m );
    int baseline = 0;
    cv::Size size = cv::getTextSize( label, cv::FONT_HERSHEY_SIMPLEX, 0.42, 1, &baseline );
    cv::putText( img, label, cv::Point( px - size.width / 2, MARGIN_T + PLOT_H + TICK_LEN + size.height + 4 ),
                 cv::FONT_HERSHEY_SIMPLEX, 0.42, cv::Scalar( 0, 0, 0 ), 1, cv::LINE_AA );
  }

  for( int m = -GRID; m <= GRID; m++ ){
    cv::Point p = toPixel( 0, m );
    int py = MARGIN_T + p.y;

    cv::line( img, cv::Point( MARGIN_L - TICK_LEN, py ), cv::Point( MARGIN_L, py ),
               cv::Scalar( 0, 0, 0 ), 1 );

    std::string label = std::to_string( m );
    int baseline = 0;
    cv::Size size = cv::getTextSize( label, cv::FONT_HERSHEY_SIMPLEX, 0.42, 1, &baseline );
    cv::putText( img, label, cv::Point( MARGIN_L - TICK_LEN - size.width - 4, py + size.height / 2 ),
                 cv::FONT_HERSHEY_SIMPLEX, 0.42, cv::Scalar( 0, 0, 0 ), 1, cv::LINE_AA );
  }

  {
    std::string title = "urg_viewer";
    int baseline = 0;
    cv::Size size = cv::getTextSize( title, cv::FONT_HERSHEY_SIMPLEX, 0.6, 1, &baseline );
    cv::putText( img, title, cv::Point( MARGIN_L + ( PLOT_W - size.width ) / 2, MARGIN_T / 2 + size.height / 2 ),
                 cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar( 0, 0, 0 ), 1, cv::LINE_AA );
  }

  {
    std::string label = "x [m]";
    int baseline = 0;
    cv::Size size = cv::getTextSize( label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseline );
    cv::putText( img, label, cv::Point( MARGIN_L + ( PLOT_W - size.width ) / 2, MARGIN_T + PLOT_H + MARGIN_B - 8 ),
                 cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar( 0, 0, 0 ), 1, cv::LINE_AA );
  }

  {
    std::string label = "y [m]";
    int baseline = 0;
    cv::Size size = cv::getTextSize( label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseline );

    cv::Mat labelImg( size.height + baseline, size.width, CV_8UC3, cv::Scalar( 255, 255, 255 ) );
    cv::putText( labelImg, label, cv::Point( 0, size.height ),
                 cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar( 0, 0, 0 ), 1, cv::LINE_AA );

    cv::Mat labelRot;
    cv::rotate( labelImg, labelRot, cv::ROTATE_90_COUNTERCLOCKWISE );

    int x = 12;
    int y = MARGIN_T + ( PLOT_H - labelRot.rows ) / 2;
    labelRot.copyTo( img( cv::Rect( x, y, labelRot.cols, labelRot.rows ) ) );
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
