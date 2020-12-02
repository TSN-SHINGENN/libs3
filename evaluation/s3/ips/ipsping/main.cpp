#include <stdio.h>
#include <stdlib.h>
#include <core/libmpxx/mpxx_unistd.h>

#include <core/libips/ips_ping.h>


int
main( int ac, char **av )
{
   int result;
   s3::ips_ping_t p;
   int id = 0;

   fprintf( stderr, "Hit enter\n"); fgetc(stdin);
//    id = getuid();
//    printf( "getuid = %d\n", getuid());
    if( id != 0 ) {
	printf( "Please execute at root user\n");
	return 1;
    }

    result = s3::ips_ping_connect(&p, av[1], 56, NULL);
    if( result ) {
	fprintf( stdout, "ping_connect fail\n");
	exit(1);
    }

    while(1) {
	s3::ips_ping_exec( &p, 10);
	mpxx_sleep(1);
    }

    return 0;
}
