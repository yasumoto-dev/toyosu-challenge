// odom_gl を読み、CSVとして標準出力に流す

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include <signal.h>

#include <ssm.hpp>

#include "utility.hpp"
#include "odom_gl.hpp"

static int gShutOff = 0;
static unsigned int dT = 5; // [ms]

static SSMApi< odom_gl > *ODM;

static void ctrlC( int aStatus );
static void setSigInt( void );
static void setupSSM( void );
static void Terminate( void );


int main( void )
{
  SSMApi< odom_gl > odm( SNAME_ODOM, 0 );
    ODM = &odm;

  try {
    setupSSM( );
    setSigInt( );

    printf( "# time[s] x[m] y[m] yaw[rad] v[m/s] w[rad/s]\n" );

    while( !gShutOff ){
      if( odm.readNew( ) ){
        printf( "%.6f %.4f %.4f %.4f %.4f %.4f\n",
                odm.time,
                odm.data.pose[ _X ], odm.data.pose[ _Y ], odm.data.pose[ _YAW ],
                odm.data.v, odm.data.w );
        fprintf( stderr, "\r%7.3f %7.3f %7.3f",
                odm.data.pose[ _X ], odm.data.pose[ _Y ], odm.data.pose[ _YAW ] );
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

  fprintf( stderr, "open odom_gl ... " );
  if( !ODM->open( SSM_READ ) ){
    throw std::runtime_error( "ERROR : fail to open odom_gl on ssm." );
  }
  fprintf( stderr, "OK.\n" );
}

static void Terminate( void )
{
  ODM->release( );
  endSSM( );
  fprintf( stderr, "\nend\n" );
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


