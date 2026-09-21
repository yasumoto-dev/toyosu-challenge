// UST-20LX のスキャンを urg_fs として配信する

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include <getopt.h>
#include <signal.h>

#include <ssm.hpp>

#include "config.hpp"
#include "lidar.hpp"

static int gShutOff = 0;
static unsigned int dT = 25;         // [ms]

static char gDevice[ 128 ];
static bool gFlagDevice = false;

static SSMApi< config_data, config_property > *CONF;
static SSMApi< urg_fs, urg_property > * URG;

static void ctrlC( int aStatus );
static void setSigInt( void );
static void printShortHelp( const char *programName );
static bool setOption( int aArgc, char *aArgv[ ] );
static void setupSSM( void );
static void Terminate( void );
static void setDevice( const urg_param *urg );


int main( int aArgc, char *aArgv[ ] )
{
  if( !setOption( aArgc, aArgv ) ) return EXIT_FAILURE;

  SSMApi< config_data, config_property > conf( SNAME_CONFIG, 0 );
    CONF = &conf;
  SSMApi< urg_fs, urg_property > urg( SNAME_URG, 0 );
    URG = &urg;

  lidarHokuyo lidar;

  try {
    setupSSM( );
    setSigInt( );

    setDevice( &conf.property.urg );
    if( !lidar.initialize( gDevice, conf.property.urg.range_min, conf.property.urg.range_max ) ){
      throw std::runtime_error( "ERROR: fail to initialize lidar." );
    }

    lidar.getProperty( &urg.property );
    if( !urg.setProperty( ) ){
      throw std::runtime_error( "ERROR: fail to set urg_fs.property." );
    }

    printf(" start urg_handler ( %s )\n", gDevice );
    while( !gShutOff ){
      if( lidar.getPointCloudData( &urg.data ) ){
        urg.write( );
      }
      usleepSSM( dT * 1000 );
    }
  }
  catch( const std::runtime_error &error ){
    fprintf( stderr, "%s\n", error.what( ) );
  }
  catch( ... ){
    fprintf( stderr, "An unknown fatal error has occured. Aborting.\n" ); 
  }

  lidar.terminate( );
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

  fprintf( stderr, "create urg_fs ... " );
  if( !URG->create( 1, ( double )dT/1000.0 ) ){
    throw std::runtime_error( "ERROR : fail to create urg_fs on ssm." );
  }
  fprintf( stderr, "OK.\n" );
}

static void Terminate( void )
{
  CONF->release( );
  URG->release( );
  endSSM( );
  printf( "\nend\n" );
}

// -d が指定されて無ければ config の ip:port を組み立てる
static void setDevice( const urg_param *urg )
{
  if( gFlagDevice ) return;

  snprintf( gDevice, sizeof( gDevice ), "%s:%d", urg->device, urg->port );
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
  fprintf( stderr, "\t$ %s -d 192.168.0.10:10940\n", programName );
  fprintf( stderr, "OPTION\n" );
  fprintf( stderr, "\t-d | --device DEVICE : Set device. ( default : use toyosu_config )\n" );
  fprintf( stderr, "\t-t | --s_time TIME   : Wait time. ( default : %d ms )\n", dT );
  fprintf( stderr, "\t-h | --help          : Show this help.\n" );
  fprintf( stderr, "\n" );
}

static bool setOption( int aArgc, char *aArgv[ ] )
{
  int opt, optIndex = 0;
  struct option longOpt[ ] = {
    { "device", 1, 0, 'd' },
    { "s_time", 1, 0, 't' },
    { "help",   0, 0, 'h' },
    { 0, 0, 0, 0 }
  };

  while( ( opt = getopt_long( aArgc, aArgv, "d:t:h", longOpt, &optIndex ) ) != -1 ){
    switch( opt ){
    case 'd':
      snprintf( gDevice, sizeof( gDevice ), "%s", optarg );
      gFlagDevice = true;
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
