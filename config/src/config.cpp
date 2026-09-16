// パラメータをSSMのpropertyとして全プロセスへ配信

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include <getopt.h>
#include <signal.h>

#include <ssm.hpp>
#include "config.hpp"
#include "param.hpp"

static int gShutOff = 0;

static char gPath[ 256 ] = "";
static unsigned int dT = 100; // [ms]

static void ctrlC( int aStatus );
static void setSigInt( void );
static void printShortHelp( const char *programName );
static bool setOption( int aArgc, char *aArgv[ ] );


int main( int aArgc, char *aArgv[ ] )
{
  if( !setOption( aArgc, aArgv ) ) return EXIT_FAILURE;

  SSMApi< config_data, config_property > conf( SNAME_CONFIG, 0 );

  if (!loadParam( gPath, &conf.property ) ) return EXIT_FAILURE;

  try {
    if( !initSSM( ) ){
      throw std::runtime_error( "ERROR : fail to initialize ssm." );
    }
    if( !conf.create( 0.5, ( double )dT/1000.0 ) ){
      throw std::runtime_error(" ERROR : fail to create config on ssm." );
    }
    if( !conf.setProperty( ) ){
      throw std::runtime_error(" ERROR : fail to set config.property." );
    }

    setSigInt( );

    // パラメータログ表示
    printParam( &conf.property );

    while( !gShutOff ){
      gShutOff = 1;
      for( int i = 0; i < 2; i++ ){
        conf.data.dummy = 999;
        conf.write( );
      }
    }
  }
  catch (const std::runtime_error &error ){
    fprintf( stderr, "%s\n", error.what( ) );
  }
  catch( ... ){
    fprintf( stderr, "An unknown fatal error has occured. Aborting.\n" );
  }

  conf.release( );
  endSSM( );
  printf( "\nend\n" );

  return EXIT_SUCCESS;
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
  fprintf( stderr, "\t$ %s -p PATH\n", programName );
  fprintf( stderr, "\t$ %s -p ../config/param/toyosu.yaml\n", programName );
  fprintf( stderr, "OPTION\n" );
  fprintf( stderr, "\t-p | --path PATH : Path of the parameter file. ( required )\n" );
  fprintf( stderr, "\n" );
}

static bool setOption( int aArgc, char *aArgv[ ] )
{
  int opt, optIndex = 0;
  struct option longOpt[ ] = {
    { "path", 1, 0, 'p' },
    { "help", 0, 0, 'h' },
    { 0, 0, 0, 0 }
  };

  while( ( opt = getopt_long( aArgc, aArgv, "p:h", longOpt, &optIndex ) ) != -1 ){
    switch( opt ){
    case 'p':
      snprintf( gPath, sizeof( gPath ), "%s", optarg );
      break;
    case 'h':
      printShortHelp( aArgv[ 0 ] );
      return false;
    default:
      fprintf( stderr, "help : %s -h\n", aArgv[ 0 ] );
      return false;
    }
  }

  if( gPath[ 0 ] == '\0' ){
    fprintf( stderr, "ERROR : parameter file is not specified.\n" );
    printShortHelp( aArgv[ 0 ] );
    return false;
  }

  return true;
}
