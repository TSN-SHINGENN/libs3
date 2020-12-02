/**
 *
 *	Basic Author: Seiichi Takeda  '2013-April-17 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @fn eia232.c
 * @brief EIA232(RS232C)準拠 シリアルインターフェースでIOするための基本ライブラリ
 */

#ifdef WIN32
/* Microsoft Windows Series */
#if _MSC_VER >= 1400            /* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#endif

#include <stdint.h>
//#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include "eia232.h"

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

typedef struct _BaudRatetable {
    union {
	struct {
	    unsigned int win32:1;
	    unsigned int linux:1;
	    unsigned int macosx:1;
	} f;
	unsigned int flags;
    } valid;
    int bourate;
} BaudRatetable_t;

static BaudRatetable_t boudtab[] = {
/*{w,l,m}, Baud */
{ {1,1,1},   110},
{ {1,1,1},   300},
{ {1,1,1},   600},
{ {1,1,1},  1200},
{ {1,1,1},  2400},
{ {1,1,1},  4800},
{ {1,1,1},  9600},
{ {1,0,0}, 14400},
{ {1,1,1}, 19200},
{ {1,0,0}, 19230}, /* FDTI AN232B-05 */
{ {1,1,1}, 38400},
{ {1,0,0}, 38461}, /* FDTI AN232B-05 */
{ {1,0,0}, 56000},
{ {1,1,1}, 57600},
{ {1,0,0}, 57692}, /* FDTI AN232B-05 */
{ {1,0,0},128000},
{ {1,1,1},115200},
{ {1,0,0},115384}, /* FDTI AN232B-05 */
{ {0,0,1},230400}, /* FDTI TN_105 */
{ {1,0,0},230769}, /* FDTI AN232B-05 */
{ {1,0,0},461538}, /* FDTI AN232B-05 */
{ {1,0,0},256000},
{ {1,0,0},923076}, /* FDTI AN232B-05 */
{ {0,0,0},    -1}};

/**
 * @fn int eia232dev_new_list_os_supported_baudrate(eia232dev_supported_baudrate_t *list_p)
 * @brief OSがサポートしているボーレートのリストを作成して戻します
 * @param list_p eia232dev_supported_boudrate_tインスタンスポインタ
 * @retval ENOMEM リソースが獲得できなかった
 * @retval -1
 * @retval 0 成功
 */
int eia232dev_new_list_os_supported_baudrate(eia232dev_supported_baudrate_t *list_p)
{
    size_t n, cnt;
    int *bp;

/**
 * @note とりあえずWindowsのみ
 */
    memset( list_p, 0x0, sizeof(eia232dev_supported_baudrate_t));

    /* 有効な値をカウント */
    cnt = 0;
    for( n=0; boudtab[n].bourate != -1; ++n) {
	if(boudtab[n].valid.f.win32) {
	    ++cnt;
	}
    }

    if(cnt==0) {
	return -1;
    }

    bp = (int*)malloc(sizeof(int) * cnt);
    if( NULL == bp ) {
	return -1;
    }

    list_p->num = cnt;
    list_p->baudrate_table = bp;
    for( n=0; boudtab[n].bourate != -1; ++n) {
	if(boudtab[n].valid.f.win32) {
	    *bp = boudtab[n].bourate;
	     bp++;
	}
    }

    return 0;
}

void eia232dev_delete_list_supported_baudrate( eia232dev_supported_baudrate_t *list_p)
{
    list_p->num = 0;
    if( NULL != list_p->baudrate_table ) {
	free(list_p->baudrate_table);
    }

    return;
}

static int comport_is_exist( const char *path)
{
    HANDLE h;

    h = CreateFile(_T(path), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if( INVALID_HANDLE_VALUE == h ) {
	return 0;
    }
    CloseHandle(h);

    return 1;
}

#if 0
eia232dev_list_t *eia232dev_new_devlist(void)
{
#define MAX_COMS 128
    unsigned int n;
    size_t size;
    const char * const comfmt = "COM%d";
    eia232dev_list_t *l;
    char devname[MAX_PATH];

    size = sizeof(eia232dev_list_t) + (sizeof(eia232dev_table_t) * MAX_COMS);
    l = (eia232dev_list_t*)malloc(size);
    if( NULL == l ) {
	return NULL;
    }
    memset( l, 0x0, size);

    for( n=0; n<MAX_COMS; ++n) {
	eia232dev_table_t *t = &l->tabs[l->num_tabs];
	_snprintf( devname, MAX_PATH, comfmt, n);
	if( comport_is_exist(devname) ) {
	    strcpy(t->path, devname);
	    ++l->num_tabs;
	}
    }	    

     return l;
}
#else
eia232dev_list_t *eia232dev_new_devlist(void)
{
    HKEY hKey = NULL;
    HDEVINFO hDevI = NULL;
    LONG lresult;
    DWORD dwnum_ports, dwlen;
    int status;
    size_t n, t, size;
    eia232dev_list_t *l=NULL;
    SP_DEVINFO_DATA DeviceInfoData = { sizeof(SP_DEVINFO_DATA)};
    char buf[256];

    /**
     * @note シリアルポートとして存在するレジストリキーを開く
     */
    lresult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("HARDWARE\\DEVICEMAP\\SERIALCOMM"), 0, KEY_READ, &hKey );
    if( lresult != ERROR_SUCCESS ) {
        status = ENOENT;
        goto out;
    }

    /**
     * @note 要素数を取得
     */
    lresult = RegQueryInfoKey( hKey, NULL, NULL, NULL, NULL, NULL, NULL, &dwnum_ports,
             NULL, NULL, NULL, NULL);
    if( lresult != ERROR_SUCCESS ) {
        status = EIO;
        goto out;
    }

    RegCloseKey(hKey);

    size = sizeof(eia232dev_list_t) + (sizeof(eia232dev_table_t) * dwnum_ports);
    l = (eia232dev_list_t*)malloc(size);
    if( NULL == l ) {
	return NULL;
    }
    memset( l, 0x0, size);

    // COMポートのデバイス情報を取得
    hDevI = SetupDiGetClassDevs( &GUID_DEVINTERFACE_COMPORT, NULL, NULL, (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));
    if( NULL == hDevI ) {
        status = ENOENT;
        goto out;
    }

    for( n=0, t=0; n<(size_t)dwnum_ports; ++n) {
        BOOL bRet;
        bRet = SetupDiEnumDeviceInfo( hDevI, n, &DeviceInfoData );
        if( bRet == FALSE) {
            DBMS4( stdout, "SetupDiEnumDeviceInfo fail\n");
            continue;
        }

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

        // デバイスインスタンスIDを取得
        bRet = SetupDiGetDeviceInstanceId( hDevI, &DeviceInfoData, buf, sizeof(buf), &dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "DeviceInstanceId = %s\n", buf);
        }
        // COMポート番号を取得
        hKey = SetupDiOpenDevRegKey( hDevI, &DeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE );
        if( NULL !=hKey ){
            eia232dev_table_t *t = &l->tabs[l->num_tabs];
            DWORD tmp_type = 0;
            dwlen = sizeof( buf );
            RegQueryValueEx( hKey, _T("PortName"), NULL, &tmp_type , buf, &dwlen );
            DBMS4( stdout, "DeviceName = %s\n", buf);
            RegCloseKey(hKey);
            hKey = NULL;
	    strcpy(t->path, buf);
	    ++l->num_tabs;
        }
    }
    status = 0;

out:
     if( NULL != hKey ) {
        RegCloseKey(hKey);
     }

     errno = status;

     return (errno == 0 ) ? l : NULL;
}

#endif

void  eia232dev_delete_devlist( eia232dev_list_t *const l )
{
    if( NULL != l ) {
	free(l);
    }

    return;
}

typedef struct _eia232dev_handle_ext {
    HANDLE h_comport;
    uint32_t timeout_msec;
} eia232dev_handle_ext_t;

int eia232dev_open( eia232dev_handle_t *const self_p, const char *const path)
{
    eia232dev_handle_ext_t *e;
    int status;
    BOOL bRet;

    memset( self_p, 0x0, sizeof(eia232dev_handle_ext_t));
    e = (eia232dev_handle_ext_t*)malloc(sizeof(eia232dev_handle_ext_t));
    if( NULL == e ) {
	return ENOMEM;
    }
    memset( e, 0x0, sizeof(eia232dev_handle_ext_t));
    self_p->ext = e; 

    /* デバイスオープン */
    e->h_comport = CreateFile(_T(path), GENERIC_READ | GENERIC_WRITE, 0, NULL,
	OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if( INVALID_HANDLE_VALUE == e->h_comport ) {
	status = ENODEV;
	goto out;
    }

    /* 送受信バッファのサイズ決定 */
    bRet = SetupComm( e->h_comport,
	//   バッファーサイズ：　受信のバッファーサイズをバイト単位で指定( YMODEM:1200. EtherNet:1600)
	1600, 1600);
    if( FALSE == bRet ) {
	DBMS1( stderr, __FUNCTION__ " : SetupComm fail\n");
	status = EACCES;
	goto out;
    }

    /* 送受信バッファ初期化 */
    bRet = PurgeComm(e->h_comport,
	//   実行する操作： 上記は未処理の読書きの中止及び送受信のバッファーのクリアを指定
	PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR );
    if( FALSE == bRet ) {
	DBMS1( stderr, __FUNCTION__ " : PurgeComm fail\n");
	status = EACCES;
	goto out;
    }

    e->timeout_msec = 0;
    status = 0;

out:
    if(status) {
	eia232dev_close(self_p);
    }

    return status;
}

void eia232dev_close( eia232dev_handle_t *self_p)
{
    eia232dev_handle_ext_t * const e = self_p->ext;
    if( NULL != e->h_comport ) {
	CloseHandle(e->h_comport);
    }

    free(self_p->ext);
    self_p->ext = NULL;

    return;
}

static int flowctrl_attr2wincom( DCB *dcb_p, const enum_eia232dev_flowtype_t flowtype)
{
    switch(flowtype) {
    case EIA232DEV_FLOW_OFF:
	dcb_p->fRtsControl = RTS_CONTROL_DISABLE;
        dcb_p->fOutxCtsFlow = FALSE;
        dcb_p->fInX = FALSE;
        dcb_p->fOutX = FALSE;
	break;
    case EIA232DEV_FLOW_HW:
	dcb_p->fRtsControl = RTS_CONTROL_HANDSHAKE;
        dcb_p->fOutxCtsFlow = TRUE;
        dcb_p->fInX = FALSE;
        dcb_p->fOutX = FALSE;
	break;
    case EIA232DEV_FLOW_XONOFF:
	dcb_p->fRtsControl = RTS_CONTROL_DISABLE;
        dcb_p->fOutxCtsFlow = FALSE;
        dcb_p->fInX = TRUE;
        dcb_p->fOutX = TRUE;
	break;
    case EIA232DEV_FLOW_unknown:
    case EIA232DEV_FLOW_eod:
    default:
	return -1;
    }
    return 0;
}

static enum_eia232dev_flowtype_t flowctrl_wincom2attr( DCB *dcb_p )
{
    if( (dcb_p->fInX == FALSE) && (dcb_p->fOutX == FALSE ) ) {
	if( dcb_p->fRtsControl == RTS_CONTROL_DISABLE ) {
	    if( dcb_p->fOutxCtsFlow == FALSE ) {
		return EIA232DEV_FLOW_OFF;
	    }
	} else if( dcb_p->fRtsControl == RTS_CONTROL_HANDSHAKE ) {
	    if(dcb_p->fOutxCtsFlow == TRUE ) {
		return EIA232DEV_FLOW_HW;
	    }
	} 
    } else if( (dcb_p->fInX == TRUE) && (dcb_p->fOutX == TRUE ) ) {
	if( ( dcb_p->fRtsControl == RTS_CONTROL_DISABLE ) && ( dcb_p->fOutxCtsFlow == FALSE )) {
	    return EIA232DEV_FLOW_XONOFF;
	}
    } 
    return EIA232DEV_FLOW_unknown;
} 
 
static enum_eia232dev_databitstype_t datebits_com2attr(const BYTE bytesize)
{
    switch(bytesize) {
    case 5:
	return EIA232DEV_DATA_5;
    case 6:
	return EIA232DEV_DATA_6;
    case 7:
	return EIA232DEV_DATA_7;
    case 8:
	return EIA232DEV_DATA_8;
    }
    return EIA232DEV_DATA_unknown;
}

static BYTE databits_attr2com(const enum_eia232dev_databitstype_t bytesize)
{
    switch(bytesize) {
    case EIA232DEV_DATA_5:
	return 5;
    case EIA232DEV_DATA_6:
	return 6;
    case EIA232DEV_DATA_7:
	return 7;
    case EIA232DEV_DATA_8:
	return 8;
    default:
	return ~0;
    }
}
static enum_eia232dev_stopbitstype_t stopbits_com2attr( BYTE StopBits)
{
    switch(StopBits) {
    case ONESTOPBIT: 
	return EIA232DEV_STOP_1;
    case TWOSTOPBITS: 
	return EIA232DEV_STOP_2;
    default:
	return EIA232DEV_STOP_unknown;
    }
}

static BYTE stopbits_attr2com( const enum_eia232dev_stopbitstype_t StopBits) 
{
    switch(StopBits) {
    case EIA232DEV_STOP_1:
	return ONESTOPBIT;
    case EIA232DEV_STOP_2:
	return TWOSTOPBITS;
    default:
	return ~0;
    }
}

static enum_eia232dev_paritytype_t parity_com2attr( BYTE Parity)
{
    switch(Parity) {
    case NOPARITY:
	return  EIA232DEV_PAR_NONE;
    case ODDPARITY:
	return  EIA232DEV_PAR_ODD;
    case EVENPARITY:
	return  EIA232DEV_PAR_EVEN;
    default:
	return  EIA232DEV_PAR_unknown;
    }
}

static BYTE parity_attr2com(enum_eia232dev_paritytype_t Parity)
{
    switch(Parity) {
    case EIA232DEV_PAR_NONE:
	return NOPARITY;
    case EIA232DEV_PAR_ODD:
	return ODDPARITY;
    case EIA232DEV_PAR_EVEN:
	return EVENPARITY;
    default:
	return ~0;
    }
}

/**
 * @fn int eia232dev_get_attr( eia232dev_handle_t *self_p, eia232dev_attr_t *attr_p)
 * @brief 現在のEIA232ポートの属性を取得します。
 * @retva 0 成功
 * @retval -1 失敗(未実装)
 **/
int eia232dev_get_attr( eia232dev_handle_t *const self_p, eia232dev_attr_t *const attr_p)
{
    eia232dev_handle_ext_t * const e = self_p->ext;
    DCB dcb;
    
    GetCommState( e->h_comport, &dcb);
    memset( attr_p, 0x0, sizeof(eia232dev_attr_t));

    attr_p->baudrate = dcb.BaudRate;
    attr_p->databits = datebits_com2attr(dcb.ByteSize);
    attr_p->stopbits = stopbits_com2attr(dcb.StopBits);
    attr_p->parity   = parity_com2attr(dcb.Parity);
    attr_p->flowctrl = flowctrl_wincom2attr(&dcb);
    attr_p->timeout_msec = e->timeout_msec;

    return 0;
}

/**
 * @fn int eia232dev_set_attr( eia232dev_handle_t *self_p, const eia232dev_attr_t *attr_p)
 * @biref EIA232ポートの属性を設定します。
 * @retva 0 成功
 * @retval -1 失敗(未実装)
 */
int eia232dev_set_attr( eia232dev_handle_t *const self_p, const eia232dev_attr_t *const attr_p)
{
    eia232dev_handle_ext_t * const e = self_p->ext;
    DCB dcb;
    
    GetCommState( e->h_comport, &dcb);

    dcb.BaudRate = attr_p->baudrate;
    dcb.ByteSize = databits_attr2com(attr_p->databits);
    dcb.StopBits = stopbits_attr2com(attr_p->stopbits);
    dcb.Parity   = parity_attr2com(attr_p->parity);
    dcb.fParity  = ( attr_p->parity != EIA232DEV_PAR_NONE ) ? TRUE : FALSE;
    
    flowctrl_attr2wincom( &dcb, attr_p->flowctrl);
    e->timeout_msec = attr_p->timeout_msec;

    SetCommState( e->h_comport, &dcb);

    return 0;
}

/**
 * @fn int eia232dev_read_stream( eia232dev_handle_t *self_p, void *buf, int bufsize, int *rlen_p)
 * @brief バイナリストリームを呼び出します。
 *	※　タイムアウト処理をまだ実装していません。
 * @retval 0 成功
 * @retval EIO 仮想レベルでIOエラーが発生した
 **/
int eia232dev_read_stream( eia232dev_handle_t *const self_p, void *buf, const size_t bufsize, size_t *const rlen_p)
{
    eia232dev_handle_ext_t * const e = self_p->ext;
    DWORD dwBytes;
    DWORD blResult;
    size_t rlen =0;

    blResult = ReadFile( e->h_comport, buf, bufsize, &dwBytes, NULL );
    if( blResult && ( dwBytes == 0)) {
	return EIO;
    }

    if(NULL != rlen_p) {
	*rlen_p = dwBytes;
    }

    return 0;
}

/**
 * @fn int eia232dev_clear_recv( eia232dev_handle_t *const self_p)
 * @brief 受信バッファのデータを排除します
 * @retval EIO とりあえずエラーが発生した
 **/
int eia232dev_clear_recv( eia232dev_handle_t *const self_p)
{
    const int TRYOUT = 10;
    eia232dev_handle_ext_t * const e = self_p->ext;
    int result;
    DWORD dwCount;
    DWORD dwErr;
    COMSTAT ComStat;
    size_t rlen =0;
    char buf[1024];
    size_t trycnt;

    while(1) {
	trycnt=0;
    	do {
	   trycnt++;
	   Sleep(100);
	   ClearCommError( e->h_comport, &dwErr, &ComStat);
	   dwCount = ComStat.cbInQue;   // キュー内のデータ数を参照
    	} while (dwCount == 0 && (trycnt != TRYOUT));

	if(trycnt == TRYOUT ) {
	    return 0;
	}
	if(!(dwCount < 1024)) {
		dwCount = 1024;
	}

	result = eia232dev_read_stream( self_p, &buf, dwCount, &rlen);
	if(result) {
	    return EIO;
	}
	dwCount -= rlen;

    }

    return 0;
}



/**
 * @fn int eia232dev_readln_string( eia232dev_handle_t *self_p, void *buf, int bufsize, int *rlen_p)
 * @brief NULL文字または改行文字までを受信します。
 *	※ 改行文字はまだ実装されていません。
 * @retval 0 成功
 * @retval EIO とりあえずエラーが発生した
 */
int eia232dev_readln_string( eia232dev_handle_t *const self_p, char *buf, const size_t bufsize, size_t *const recvlen_p)
{
    eia232dev_handle_ext_t * const e = self_p->ext;
    DWORD dwCount;
    DWORD dwErr;
    COMSTAT ComStat;
    int result;
    size_t recvlen =0, recv;
    char *pbuf=(char*)buf;
    int status;

    while(1) {
    	do {
	   ClearCommError( e->h_comport, &dwErr, &ComStat);
	   dwCount = ComStat.cbInQue;   // キュー内のデータ数を参照
    	} while (dwCount == 0 );

	result = eia232dev_read_stream( self_p, pbuf, 1, &recv);
	if(result) {
	    return EIO;
	}

	if(recv == 0) {
	    status = 0;
	    goto out;
	}

	if( '\0' == *pbuf) {
	    status = 0;
	    goto out;
	} else if( NULL != strchr( "\r\n", (int)(*pbuf))) {
#if 0
		*pbuf = '\n';
		status = 0;
	    goto out;
#endif
	}
	++pbuf;
 	++recvlen;
    }
out:  
    if( NULL != recvlen_p ) {
	*recvlen_p = recvlen;
    }

    return status;
}

const char *eia232dev_databitstype2str(enum_eia232dev_databitstype_t databits)
{
/* *INDENT-OFF* */
    struct {
	enum_eia232dev_databitstype_t id;
	const char *tag;
    } tabs[] = {
	{ EIA232DEV_DATA_5, "5bits"},
	{ EIA232DEV_DATA_6, "6bits"},
	{ EIA232DEV_DATA_7, "7bits"},
	{ EIA232DEV_DATA_8, "8bits"},
	{ EIA232DEV_DATA_unknown, "unknown"},
	{ EIA232DEV_DATA_eod, NULL}
    };
/* *INDENT-ON* */
    size_t n;

    for( n=0; tabs[n].tag != NULL; ++n) {
	if( databits == tabs[n].id ) {
	    return tabs[n].tag;
        }
    }

    return NULL;
}

const char *eia232dev_paritytype2str(enum_eia232dev_paritytype_t paritytype)
{
/* *INDENT-OFF* */
    struct {
	enum_eia232dev_paritytype_t id;
	const char *tag;
    } tabs[] = {
	{ EIA232DEV_PAR_NONE   , "NONE"},
	{ EIA232DEV_PAR_ODD    , "ODD"},
	{ EIA232DEV_PAR_EVEN   , "EVEN"},
	{ EIA232DEV_PAR_unknown, "unknown"},
	{ EIA232DEV_PAR_eod    , NULL}
    };
/* *INDENT-ON* */
    size_t n;

    for( n=0; tabs[n].tag != NULL; ++n) {
	if( paritytype == tabs[n].id ) {
	    return tabs[n].tag;
        }
    }

    return NULL;
}

const char *eia232dev_stopbitstype2str(enum_eia232dev_stopbitstype_t stopbits) {
/* *INDENT-OFF* */
    struct {
	enum_eia232dev_stopbitstype_t id;
	const char *tag;
    } tabs[] = {
	{ EIA232DEV_STOP_1, "1bit"},
	{ EIA232DEV_STOP_2, "2bit"},
	{ EIA232DEV_STOP_unknown, "unknown"},
	{ EIA232DEV_STOP_eod   , NULL}
    };
/* *INDENT-ON* */

    size_t n;

    for( n=0; tabs[n].tag != NULL; ++n) {
	if( stopbits == tabs[n].id ) {
	    return tabs[n].tag;
        }
    }

    return NULL;
}

const char *eia232dev_flowtype2str(enum_eia232dev_flowtype_t flowtype) {
/* *INDENT-OFF* */
    struct {
	enum_eia232dev_stopbitstype_t id;
	const char *tag;
    } tabs[] = {
	{ EIA232DEV_FLOW_OFF, "OFF"},
	{ EIA232DEV_FLOW_HW , "HW"},
	{ EIA232DEV_FLOW_XONOFF, "XONOFF"},
	{ EIA232DEV_FLOW_unknown, "unknown"},
	{ EIA232DEV_FLOW_eod    , NULL}
    };
/* *INDENT-ON* */
    size_t n;

    for( n=0; tabs[n].tag != NULL; ++n) {
	if( flowtype == tabs[n].id ) {
	    return tabs[n].tag;
        }
    }

    return NULL;
}

int eia232dev_write_stream( eia232dev_handle_t *const self_p, const void *dat, const size_t sendlen)
{
   BOOL bRet;
   DWORD dwRetval;
   eia232dev_handle_ext_t * const e = self_p->ext;

   bRet = WriteFile(e->h_comport, dat, (DWORD)sendlen, &dwRetval, NULL); 
   if(!bRet) {
        /* エラー処理 */
        return EIO;
    }
    if( dwRetval != sendlen ) {
        return EIO;
    }

    return 0;
}

/**
 * @fn int eia232dev_write_string( eia232dev_handle_t *const self_p, const char *const str, size_t *const retlen_p )
 * @brief 文字列を送信する
 * @param self_p eia232dev_handle_tインスタンス構造体ポインタ
 * @param retlen_p 送信済みサイズ（終端文字含む)
 * @retval 0 成功
 * @retvao EIO I/O失敗
 **/
int eia232dev_write_string( eia232dev_handle_t *const self_p, const char *const str, size_t *const retlen_p )
{
    int result;
    const size_t sendlen = strlen(str) + 1;

    result = eia232dev_write_stream(self_p, (void*)str, sendlen);
    if(result) {
	return result;
    }

    if(NULL != retlen_p) {
	*retlen_p = sendlen;
    }

    return result;
}

/**
 * @fn int eia232dev_string_command_execute( eia232dev_handle_t *const self_p, const char *send_cmd, char *recv_str, const size_t szofrecv)
 * @brief 文字列コマンドを送信する
 * @param self_p eia232dev_handle_tインスタンス構造体ポインタ
 * @param send_cmd 送信コマンド文字列ポインタ
 * @param recv_str 受信文字列取得バッファ
 * @param szofresv 受信バッファサイズ
 **/
int eia232dev_string_command_execute( eia232dev_handle_t *const self_p, const char *send_cmd, char *recv_str, const size_t bufszofrecv)
{
    int result;
    size_t retlen;

    result = eia232dev_write_string( self_p, send_cmd, &retlen);
    if(result) {
	return result;
    }

    result = eia232dev_readln_string( self_p, recv_str, bufszofrecv, &retlen);
    if(result) { 
	return result;
    }

    return 0;
}
