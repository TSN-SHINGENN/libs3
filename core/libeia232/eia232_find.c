/**
 *	Copyright 2013 Keisoku Giken CO.,LTD. VW Sect.
 *
 *	Basic Author: Seiichi Takeda  '2013-March-07 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @fn eia232_find.c
 * @brief ベンダID、プロダクトIDから対応するシリアルインターフェース名を検索するクラス
 */

#ifdef WIN32
/* Microsoft Windows Series */
#if _MSC_VER >= 1400            /* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#endif

#include <stdio.h>
#include <errno.h>
#include <stdint.h>

#include <libmultios/multios_string.h>
#include <libmultios/multios_gof_cmd.h>

/* this */
#include "eia232.h"
#include "eia232_find.h"

#if defined(WIN32)
#include <tchar.h>
#include <windows.h>
#include <setupapi.h>
#pragma comment(lib, "setupapi.lib")
#endif

#ifdef DEBUG
static int debuglevel = 4;
#else
static int debuglevel = 4;
#endif
#include "dbms.h"

typedef struct _win32_eia232dev_find_device_ext {
    size_t num_ports;
} win32_eia232dev_find_device_ext_t;

/**
 * @fn int eia232dev_find_device_init( eia232dev_find_device_t *self_p)
 * @brief eia232dev_find_device_tインスタンスを初期化します。
 * @param self_p eia232dev_find_device_tインスタンスポインタ
 * @retval 0 成功
 * @retval ENOMEM リソースを獲得できなかった
 * @retval それ以外 エラーだけと未定義
 */
int eia232dev_find_device_init( eia232dev_find_device_t *self_p)
{
    win32_eia232dev_find_device_ext_t *e;

    memset(self_p, 0x0, sizeof(eia232dev_find_device_t));

    e = (win32_eia232dev_find_device_ext_t*)malloc(sizeof(win32_eia232dev_find_device_ext_t));
    if( NULL == e ) {
	return ENOMEM;
    }
    memset( e, 0x0, sizeof(win32_eia232dev_find_device_ext_t));

    e->num_ports = ~0;
    self_p->ext = (void*)e;

    return 0;
}

/**
 * @fn int eia232dev_find_device_reset_instance(eia232dev_find_device_t *self_p);
 * @brief eia232dev_find_device_tインスタンスのデバイス検索位置を先頭に移動します
 * @param self_p eia232dev_find_device_tインスタンスポインタ
 * @retval 0 成功
 * @retval それ以外 エラーだけと未定義
 */
int eia232dev_find_device_reset_instance(eia232dev_find_device_t *self_p)
{
    win32_eia232dev_find_device_ext_t * const e = self_p->ext;
    e->num_ports = ~0;
    self_p->index_point = 0;
    self_p->num_found_device = 0;

    return 0;
}

/**
 * @fn int eia232dev_find_device_exec( eia232dev_find_device_t *self_p, uint32_t vid,  uint32_t pid, char *name_buf, int bufsize, int num_founds);
 * @brief 指定されたシリアルポートを検索します。(インスタンス初期化後にPnPが処理された場合、うまく認識できないことがあります。)
 * @param self_p eia232dev_find_device_tインスタンスポインタ
 * @param vid ベンダーID番号
 * @param pid プロダクトID番号
 * @param name_buf デバイスファイル名登録バッファポインタ
 * @param bufsize name_bufのバッファサイズ
 * @retval 0 見つけた
 * @retval ENOMEM bufsizeが必要サイズまで足りなかった
 * @retval ENODEV 見つからない（検出終了点に到達)
 * @retval EIO システムエラーが発生した(この後の検索処理は不安定になる)
 */
int eia232dev_find_device_exec( eia232dev_find_device_t *self_p, uint32_t vid,  uint32_t pid, char *name_buf, int bufsize)
{
    win32_eia232dev_find_device_ext_t * const e = self_p->ext;
    HKEY hKey = NULL;
    HDEVINFO hDevI = NULL;
    DWORD dwlen;
    size_t n;
    int status, result;
    char buf[512];

    if( e->num_ports == ~0 ) {
	/* WIN32の場合 */
	LONG lresult;
	DWORD dwnum_ports;

	/* シリアルポートとして存在するレジストリキーを開く */
	lresult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("HARDWARE\\DEVICEMAP\\SERIALCOMM"), 0, KEY_READ, &hKey );
	if( lresult != ERROR_SUCCESS ) {
	    status = ENODEV;
	    goto out;
	}

	/**
	 * @note 要素数を取得(デバイスでないものも含まれる
	 */
	lresult = RegQueryInfoKey( hKey, NULL, NULL, NULL, NULL, NULL, NULL, &dwnum_ports,
             NULL, NULL, NULL, NULL);
	if( lresult != ERROR_SUCCESS ) {
	    status = EIO;
	    goto out;
	}

	RegCloseKey(hKey);
	hKey = NULL;

	e->num_ports = dwnum_ports;
    }

    if( e->num_ports == 0 ) {
	status = ENODEV;
	goto out;
    }

    // レジストリに登録されているCOMポートのデバイス情報を取得
    hDevI = SetupDiGetClassDevs( &GUID_DEVINTERFACE_COMPORT, NULL, NULL, (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));
    if( NULL == hDevI ) {
        status = EIO;
        goto out;
    }

    for( n=self_p->index_point; n<e->num_ports; ++n) {
        BOOL bRet;
	SP_DEVINFO_DATA DeviceInfoData = { sizeof(SP_DEVINFO_DATA)};
        bRet = SetupDiEnumDeviceInfo( hDevI, n, &DeviceInfoData );
        if( bRet == FALSE) {
            DBMS4( stdout, "SetupDiEnumDeviceInfo fail\n");
            continue;
        }
#if 0
        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_ADDRESS, NULL, buf,sizeof(buf),&dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_ADDRESS = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_BUSNUMBER, NULL, buf, sizeof(buf),&dwlen);

        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_BUSNUMBER = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_CAPABILITIES, NULL, buf, sizeof(buf),&dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_CAPABILITIES = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_CHARACTERISTICS, NULL, buf,sizeof(buf),&dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_CHARACTERISTICS = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_CLASS, NULL, buf,sizeof(buf),&dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_CLASS = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_CLASSGUID, NULL, buf,sizeof(buf),&dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_CLASSGUID = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_COMPATIBLEIDS, NULL, buf, sizeof(buf), &dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_COMPATIBLEIDS = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_CONFIGFLAGS, NULL, buf, sizeof(buf), &dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_CONFIGFLAGS = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_DEVICE_POWER_DATA, NULL, buf, sizeof(buf), &dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_CONFIGFLAGS = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_DEVICEDESC, NULL, buf, sizeof(buf), &dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_DEVICEDESC = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_DEVTYPE, NULL, buf,sizeof(buf),&dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_DEVTYPE = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_DRIVER, NULL, buf,sizeof(buf),&dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_DRIVER = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_ENUMERATOR_NAME, NULL, buf,sizeof(buf),&dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_ENUMERATOR_NAME = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_EXCLUSIVE, NULL, buf,sizeof(buf),&dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_EXCLUSIVE = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_FRIENDLYNAME, NULL, buf,sizeof(buf),&dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_FRIENDRYNAME = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_HARDWAREID, NULL, buf,sizeof(buf),&dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_HARDWAREID = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_INSTALL_STATE, NULL, buf,sizeof(buf),&dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_INSTALL_STATE = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_LEGACYBUSTYPE, NULL, buf,sizeof(buf),&dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_LEGACYBUSTYPE = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_LOCATION_INFORMATION, NULL, buf,sizeof(buf),&dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_LOCATION_INFORMATION = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_LOCATION_PATHS, NULL, buf,sizeof(buf),&dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_LOCATION_PATHS = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_LOWERFILTERS, NULL, buf,sizeof(buf),&dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_LOWERFILTERS = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_MFG, NULL, buf,sizeof(buf),&dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_MFG = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_PHYSICAL_DEVICE_OBJECT_NAME, NULL, buf,sizeof(buf), &dwlen);
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_PHYSICAL_DEVICE_OBJECT_NAME = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_SECURITY, NULL, buf,sizeof(buf),&dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_SECURITY = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_SECURITY_SDS, NULL, buf,sizeof(buf),&dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_SECURITY_SDS = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_SERVICE, NULL, buf,sizeof(buf),&dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_SERVICE = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_UI_NUMBER_DESC_FORMAT, NULL, buf,sizeof(buf),&dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_UI_NUMBER_DESC_FORMAT = %s\n", buf);
        }

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_UPPERFILTERS, NULL, buf,sizeof(buf),&dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "SPDRP_UPPERFILTERS = %s\n", buf);
        }
#endif

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_HARDWAREID, NULL, buf,sizeof(buf),&dwlen );
        if(bRet!=TRUE) {
	    return EIO;
        }

	if(!multios_strncasecmp( buf, "ACPI", 4)) {
	    /* オンボードデバイス */
	    continue;
	}

	if(!multios_strncasecmp( buf, "USB", 3)) {
	    multios_gof_cmd_makeargv_t marg;
	    size_t c;
	    uint32_t dvid, dpid;
	    dvid = dpid = ~0;

	    /* USBデバイス */
	    result = multios_gof_cmd_makeargv_init( &marg,"\\&_", '\0', NULL);
	    if(result) {
		return EIO;
	    }
	    result = multios_gof_cmd_makeargv( &marg, buf);
	    if(result) {
		return EIO;
	    }
	    for( c=0; c<marg.argc; ++c) {
		if(!multios_strcasecmp( marg.argv[c], "VID")) {
		    if( sscanf( marg.argv[c+1], "%x", &dvid) != 1 ) {
			return EIO;
		    }
		    ++c;
		    continue;
		}
		if(!multios_strcasecmp( marg.argv[c], "PID")) {
		    if( sscanf( marg.argv[c+1], "%x", &dpid) != 1 ) {
			return EIO;
		    }
		    ++c;
		    continue;
		}
	    }
	    if( (dpid == ~0) || (dvid == ~0 ) ) {
		return EIO;
	    }
	    multios_gof_cmd_makeargv_destroy(&marg);
	    if( (vid == dvid) && ( pid == dpid ) ) {
		/* found */
	        // デバイスインスタンスIDを取得
	        bRet = SetupDiGetDeviceInstanceId( hDevI, &DeviceInfoData, buf, sizeof(buf), &dwlen );
		if(bRet==TRUE) {
		    DBMS4( stdout, "DeviceInstanceId = %s\n", buf);
		}
		// COMポート番号を取得
		hKey = SetupDiOpenDevRegKey( hDevI, &DeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE );
		if( NULL !=hKey ){
		    DWORD tmp_type = 0;
		    dwlen = sizeof( buf );
		    RegQueryValueEx( hKey, _T("PortName"), NULL, &tmp_type , buf, &dwlen );
		    DBMS4( stdout, "DeviceName = %s\n", buf);
		    RegCloseKey(hKey);
		    hKey = NULL;
		    if( NULL != name_buf ) {
			strcpy( name_buf, buf);
			++self_p->num_found_device;
			self_p->index_point = n + 1;
			status = 0;
			goto out;
		    }
		}
	    }
        }
    }
    self_p->index_point = n;

    if( n== e->num_ports ) {
	/* 最後まで到達した */
	status = ENODEV;
	goto out;
    }

    status = 0;

out:

    if( NULL != hKey ) {
	RegCloseKey(hKey);
    }

    if( NULL != hDevI ) {
	SetupDiDestroyDeviceInfoList(hDevI);
    }

    return status;
}

/**
 * @fn void eia232dev_find_device_destroy( eia232dev_find_device_t *self_p)
 * @brief eia232dev_find_device_tインスタンスを破棄します。
 */
void eia232dev_find_device_destroy( eia232dev_find_device_t *self_p)
{
    if( NULL != self_p ) {
	free(self_p->ext);
	self_p->ext = NULL;
    }

    return;
}

