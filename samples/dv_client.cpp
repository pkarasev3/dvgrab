
/*
* main.cc -- A DV/1394 capture utility
* Copyright (C) 2000-2002 Arne Schirmacher <arne@schirmacher.de>
* Copyright (C) 2003-2008 Dan Dennedy <dan@dennedy.org>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software Foundation,
* Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/** the dvgrab main program
 
    contains the main logic
*/

#include "config.h"


// C++ includes

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/program_options.hpp>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/thread.hpp>

#include <string>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace cv;
using namespace std;
using boost::lexical_cast;
using boost::asio::ip::tcp;
namespace po = boost::program_options;

namespace {

enum { max_length = 1024 };
  
struct Options {
  string port;
  string hostname;
};

int options(int ac, char ** av, Options& opts) {
  // Declare the supported options.
  po::options_description desc("Allowed options");
  desc.add_options()
      ("help", "Produce help message.")
      ("port,p", po::value<string>(&opts.port)->default_value("5001"),"port")
      ("hostname,H", po::value<string>(&opts.hostname)->default_value("localhost"),"host name or ip address");
  po::variables_map vm;
  po::store(po::parse_command_line(ac, av, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    cout << desc << "\n";
    return 1;
  }

  return 0;

}

} // end app-local namespace


// C includes

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <sys/time.h> 
#include <sys/resource.h>
#include <sys/mman.h>

// local includes

#include "io.h"
#include "dvgrab.h"
#include "error.h"

bool g_done = false;

void signal_handler( int sig )
{
	g_done = true;
}

int rt_raisepri (int pri)
{
#ifdef _SC_PRIORITY_SCHEDULING
	struct sched_param scp;

	/*
	 * Verify that scheduling is available
	 */
	if ( sysconf( _SC_PRIORITY_SCHEDULING ) == -1)
	{
// 		sendEvent( "Warning: RR-scheduler not available, disabling." );
		return -1;
	}
	else 
	{
		memset( &scp, '\0', sizeof( scp ) );
		scp.sched_priority = sched_get_priority_max( SCHED_RR ) - pri;
		if ( sched_setscheduler( 0, SCHED_RR, &scp ) < 0 )
		{
// 			sendEvent( "Warning: Cannot set RR-scheduler" );
			return -1;
		}
	}
	return 0;

#else
	return -1;
#endif
}

int main( int argc, char *argv[] )
{
  Options opts;
  if (options(argc, argv, opts)) // failed to parse!
    return 1;
	
  DVgrab::m_tcp_host = opts.hostname;
  DVgrab::m_tcp_port = opts.port;
  DVgrab::m_send_tcp_packet_frame = true;
  
  int ret = 0;

	fcntl( fileno( stderr ), F_SETFL, O_NONBLOCK );
	try
	{
		char c;
    char* client_args[1];
    client_args[0] = "client_mode";
		DVgrab dvgrab( 1, client_args );

		signal( SIGINT, signal_handler );
		signal( SIGTERM, signal_handler );
		signal( SIGHUP, signal_handler );
		signal( SIGPIPE, signal_handler );

		if ( rt_raisepri( 1 ) != 0 )
			setpriority( PRIO_PROCESS, 0, -20 );

#if _POSIX_MEMLOCK > 0
		mlockall( MCL_CURRENT | MCL_FUTURE );
#endif

		if ( dvgrab.isInteractive() )
		{
			term_init();
			fprintf( stderr, "Going interactive. Press '?' for help.\n" );
			while ( !g_done )
			{
				dvgrab.status( );
				if ( ( c = term_read() ) != -1 )
					if ( !dvgrab.execute( c ) )
						break;
			}
			term_exit();
		}
		else
		{
			dvgrab.startCapture();
			while ( !g_done )
				if ( dvgrab.done() )
					break;
			dvgrab.stopCapture();
		}
	}
	catch ( std::string s )
	{
		fprintf( stderr, "Error: %s\n", s.c_str() );
		fflush( stderr );
		ret = 1;
	}
	catch ( ... )
	{
		fprintf( stderr, "Error: unknown\n" );
		fflush( stderr );
		ret = 1;
	}

	fprintf( stderr, "\n" );
	return ret;
}




/////////////////////



#if 0
int main(int argc, char** argv) {
  
  VideoCapture capture( opts.videoN ); // at /dev/videoN
  capture.set(CV_CAP_PROP_FRAME_WIDTH,opts.frameWidth);
  capture.set(CV_CAP_PROP_FRAME_HEIGHT,opts.frameHeight);
  
  if (!capture.isOpened()) {
    cerr << "unable to open video device " << opts.videoN << endl;
    return 1;
  }

  for( ;; )
  {
    
    for (;;)   // Main Loop
    {
      try {
        capture >> frame; // grab frame data from webcam
        if (frame.empty())
          continue;
        string header = "0123456789ABCD";
        memcpy((void*)&(raw_data[0]),(void*) header.c_str(),     header_sz * nB );
        memcpy((void*)&(raw_data[header_sz]),(void*)(frame.ptr<unsigned char>(0)), n_pixels * nB );

        // write to the server, process the frameBuffer
        boost::asio::write(s, boost::asio::buffer( &(raw_data[0]), raw_data.size() ) );

        char reply[max_length];
        size_t reply_length = boost::asio::read(s,boost::asio::buffer(reply, output_sz*nB));
        if( reply_length == 4 ) {
          std::cout << "Reply is: ";
          short xy[2];
          memcpy( &(xy[0]),&(reply[0]),sizeof(short) );
          memcpy( &(xy[1]),&(reply[2]),sizeof(short) );
          std::cout << "x,y = " << xy[0] << "," << xy[1] << endl;
        } else {
          cout << "warning, bogus reply_length..." << endl;
        }

      } catch (std::exception& e) { // something crazy during network transfer
        std::cerr << "Exception: " << e.what() << "\n";
        exit(0);
      }
      char key = waitKey(10);
      if( 'q' == key ) { // quit if we hit q key
        exit(0);
      }
    }
  }
  return 0;



}
#endif 
