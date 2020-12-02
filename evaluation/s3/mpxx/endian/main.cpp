/**
 * @file main.c
 * @brief windowsテストプログラム
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <core/libmpxx/mpxx_endian.hpp>
//#include <core/libpmx/pmx_platform.h>

static void endian_check(void)
{
    const uint16_t u16=0x3210;
    const uint32_t u32=0x76543210;
    const uint64_t u64=0xfedcba9876543210;
    uint16_t be16, le16;
    uint32_t be32, le32;
    uint32_t be24, le24;
    uint64_t be48, le48;
    uint64_t be56, le56;
    uint64_t be64, le64;

    be48 = le48 = be56 = le56 = 0;
    be24 = le24 = 0;

#if defined(__LITTLE_ENDIAN__)
    fprintf( stderr, "Endian is Little\n");
#endif
    
#if defined(__BIG_ENDIAN__)
    fprintf( stderr, "Endian is Big\n");
#endif

    fprintf( stderr, "\n");

    fprintf( stderr, "u64=%llx GET BE64=0x%llx LE64=0x%llx\n",
	(long long)u64, (long long)(MPXX_GET_BE_D64(&u64)), (long long)(MPXX_GET_LE_D64(&u64)));

    fprintf( stderr, "u64=%llx GET BE56=0x%llx LE56=0x%llx\n",
	(long long)u64, (long long)(MPXX_GET_BE_D56(&u64)), (long long)(MPXX_GET_LE_D56(&u64)));

    fprintf( stderr, "u64=%llx GET BE48=0x%llx LE48=0x%llx\n",
	(long long)u64, (long long)(MPXX_GET_BE_D48(&u64)), (long long)(MPXX_GET_LE_D48(&u64)));

    fprintf( stderr, "u32=%llx GET BE32=0x%llx LE32=0x%llx\n",
	(long long)u32, (long long)(MPXX_GET_BE_D32(&u32)), (long long)MPXX_GET_LE_D32(&u32));

    fprintf( stderr, "u32=%llx GET BE24=0x%llx LE24=0x%llx\n",
	(long long)u32, (long long)(MPXX_GET_BE_D24(&u32)), (long long)MPXX_GET_LE_D24(&u32));

    fprintf( stderr, "u16=%llx GET BE16=0x%llx LE16=0x%llx\n",
	(long long)u16, (long long)(MPXX_GET_BE_D16(&u16)), (long long)MPXX_GET_LE_D16(&u16));

    MPXX_SET_BE_D64( &be64, u64);
    MPXX_SET_LE_D64( &le64, u64);

    MPXX_SET_BE_D56( &be56, u64);
    MPXX_SET_LE_D56( &le56, u64);

    MPXX_SET_BE_D48( &be48, u64);
    MPXX_SET_LE_D48( &le48, u64);

    MPXX_SET_BE_D32( &be32, u32);
    MPXX_SET_LE_D32( &le32, u32);

    MPXX_SET_BE_D24( &be24, u32);
    MPXX_SET_LE_D24( &le24, u32);

    MPXX_SET_BE_D16( &be16, u16);
    MPXX_SET_LE_D16( &le16, u16);

    fprintf( stderr, "\n");

    fprintf( stderr, "u64=%llx SET BE64=0x%llx LE64=0x%llx\n",
	(long long)u64, (long long)be64, (long long)le64); 

    fprintf( stderr, "u64=%llx SET BE56=0x%llx LE56=0x%llx\n",
	(long long)u64, (long long)be56, (long long)le56); 

    fprintf( stderr, "u64=%llx SET BE48=0x%llx LE48=0x%llx\n",
	(long long)u64, (long long)be48, (long long)le48); 

    fprintf( stderr, "u32=%llx SET BE32=0x%llx LE32=0x%llx\n",
	(long long)u32, (long long)be32, (long long)le32); 

    fprintf( stderr, "u32=%llx SET BE24=0x%llx LE24=0x%llx\n",
	(long long)u32, (long long)be24, (long long)le24); 

    fprintf( stderr, "u16=%llx SET BE16=0x%llx LE16=0x%llx\n",
	(long long)u16, (long long)be16, (long long)le16); 

    return;
}

int
main( int ac, char **av)
{
    int result;

#if 0
    result = multios_platform_is_ms_windows();
    if(result) {
	int majver, minver;
	fprintf( stderr, "OS is Windows\n");

  	result =  multios_platform_get_ms_windows_nt_version(&majver, &minver);
	if( result ) {
	    fprintf( stderr, "multios_platform_get_ms_windows_nt_version : fail\n");
	    goto out;
	}
	fprintf( stderr, "NT Version is major:%d minor:%d\n", majver, minver);

	result = multios_platform_is_ms_windows_7();
	if(result) {
	    fprintf( stderr, "is windows7\n");
	    goto out;
	}

	result = multios_platform_is_ms_windows_vista();
	if(result) {
	    fprintf( stderr, "is windows vista\n");
	    goto out;
	}

	result = multios_platform_is_ms_windows_xp();
	if(result) {
	    fprintf( stderr, "is windows XP\n");
	    goto out;
	}

	result = multios_platform_is_ms_windows_2k();
	if(result) {
	    fprintf( stderr, "is windows 2K\n");
	    goto out;
	}

	result = multios_platform_is_ms_windows_2003Server();
	if(result) {
	    fprintf( stderr, "is windows 2003Server\n");
	    goto out;
	}

	result = multios_platform_is_ms_windows_nt();
	if(result) {
	    fprintf( stderr, "is windowsNT\n");
	    goto out;
	}

	result = multios_platform_is_ms_windows_9x();
	if(result) {
	    fprintf( stderr, "is windows 9X\n");
	    goto out;
	}
    }

    result = multios_platform_is_linux();
    if( result ) {
	int majver, minver, rev;
	fprintf( stderr, "OS is Linux\n");

  	result =  multios_platform_get_linux_version(&majver, &minver, &rev);
	if( result ) {
	    fprintf( stderr, "multios_platform_get_linux_version : fail\n");
	    goto out;
	}
	fprintf( stderr, "Linux Version is major:%d minor:%d rev:%d\n", majver, minver, rev);
    }

    result = multios_platofrm_is_qnx();
    if( result ) {
	int majver, minver, rev;
	fprintf( stderr, "OS is QNX\n");

  	result =  multios_platform_get_qnx_version(&majver, &minver, &rev);
	if( result ) {
	    fprintf( stderr, "multios_platform_get_qnx_version : fail\n");
	    goto out;
	}
	fprintf( stderr, "QNX Version is major:%d minor:%d rev:%d\n", majver, minver, rev);
    }

    result = multios_platform_is_macosx();
    if( result ) {
	int majver, minver, rev;
	fprintf( stderr, "OS is MAXOSX\n");

  	result =  multios_platform_get_macosx_version(&majver, &minver, &rev);
	if( result ) {
	    fprintf( stderr, "multios_platform_get_macosx_version : fail\n");
	    goto out;
	}
	fprintf( stderr, "MACOSX Version is major:%d minor:%d rev:%d\n", majver, minver, rev);
    }

    result = multios_platform_is_sunos();
    if( result ) {
	int majver, minver;
	fprintf( stderr, "OS is SunOS\n");

  	result =  multios_platform_get_sunos_version(&majver, &minver);
	if( result ) {
	    fprintf( stderr, "multios_platform_get_sunos_version : fail\n");
	    goto out;
	}
	fprintf( stderr, "SunOS Version is major:%d minor:%d\n", majver, minver);
    }
#endif

out:
    endian_check();

    fgetc(stdin);

    return 0;

}
