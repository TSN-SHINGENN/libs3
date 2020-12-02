/**
 *
 *	Basic Author: Seiichi Takeda  '2013-April-17 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @fn eia232.c
 * @brief EIA232(RS232C)���� �V���A���C���^�[�t�F�[�X��IO���邽�߂̊�{���C�u����
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

#if defined(WIN32)
#include <tchar.h>
#include <windows.h>
#include <setupapi.h>
#pragma comment(lib, "setupapi.lib")
#include <cfgmgr32.h>
#pragma comment(lib, "cfgmgr32.lib")

#endif

/* libs3 */
#include "libmpxx/mpxx_sys_types.h"

#ifdef DEBUG
static int debuglevel = 4;
#else
static const int debuglevel = 0;
#endif
#include "libmpxx/dbms.h"

/* this */
#include "mcxx_dev232.h"

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
 * @fn int s3::mcxx_dev232_new_list_os_supported_baudrate(s3::mcxx_dev232_supported_baudrate_t *list_p)
 * @brief OS���T�|�[�g���Ă���{�[���[�g�̃��X�g���쐬���Ė߂��܂�
 * @param list_p s3::mcxx_dev232_supported_boudrate_t�C���X�^���X�|�C���^
 * @retval ENOMEM ���\�[�X���l���ł��Ȃ�����
 * @retval -1
 * @retval 0 ����
 */
int s3::mcxx_dev232_new_list_os_supported_baudrate(s3::mcxx_dev232_supported_baudrate_t *list_p)
{
    size_t n, cnt;
    int *bp;

/**
 * @note �Ƃ肠����Windows�̂�
 */
    memset( list_p, 0x0, sizeof(s3::mcxx_dev232_supported_baudrate_t));

    /* �L���Ȓl���J�E���g */
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

void s3::mcxx_dev232_delete_list_supported_baudrate( s3::mcxx_dev232_supported_baudrate_t *list_p)
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
s3::mcxx_dev232_list_t *s3::mcxx_dev232_new_devlist(void)
{
#define MAX_COMS 128
    unsigned int n;
    size_t size;
    const char * const comfmt = "COM%d";
    s3::mcxx_dev232_list_t *l;
    char devname[MAX_PATH];

    size = sizeof(s3::mcxx_dev232_list_t) + (sizeof(s3::mcxx_dev232_table_t) * MAX_COMS);
    l = (s3::mcxx_dev232_list_t*)malloc(size);
    if( NULL == l ) {
	return NULL;
    }
    memset( l, 0x0, size);

    for( n=0; n<MAX_COMS; ++n) {
	    s3::mcxx_dev232_table_t *t = &l->tabs[l->num_tabs];
	_snprintf( devname, MAX_PATH, comfmt, n);
	if( comport_is_exist(devname) ) {
	    strcpy(t->path, devname);
	    ++l->num_tabs;
	}
    }	    

     return l;
}
#else
s3::mcxx_dev232_list_t *s3::mcxx_dev232_new_devlist(void)
{
    HKEY hKey = NULL;
    HDEVINFO hDevI = NULL;
    // LONG lresult;
    DWORD dwlen;
    int status;
    size_t n, size;
    s3::mcxx_dev232_list_t *l=NULL;
    SP_DEVINFO_DATA DeviceInfoData = { sizeof(SP_DEVINFO_DATA)};
    BYTE buf[256];

    size = sizeof(s3::mcxx_dev232_list_t);
    l = (s3::mcxx_dev232_list_t*)malloc(size);
    if( NULL == l ) {
	errno = ENOMEM;
	return NULL;
    }
    memset( l, 0x0, size);

    // COM�|�[�g�̃f�o�C�X�����擾
    GUID guidClass = GUID_NULL;
    DWORD dwRequiredSize = 0;

    SetupDiClassGuidsFromName(TEXT("Ports"), &guidClass, 1, &dwRequiredSize);
    hDevI = SetupDiGetClassDevs(&guidClass, NULL, NULL, DIGCF_PRESENT);

    for( n=0;; ++n) {
        BOOL bRet;
        bRet = SetupDiEnumDeviceInfo( hDevI, n, &DeviceInfoData );
        if( bRet == FALSE) {
	    if(GetLastError() ==  ERROR_NO_MORE_ITEMS ) {
		if(l->num_tabs == 0 ) {
		    status = ENODEV;
		    goto out;
		} else {
		    break;
		}
	    }
            DBMS4( stdout, "SetupDiEnumDeviceInfo fail, n=%d errorcode=%dn", (int)n, GetLastError());
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

        // �f�o�C�X�C���X�^���XID���擾
        bRet = SetupDiGetDeviceInstanceId( hDevI, &DeviceInfoData, (PSTR)buf, sizeof(buf), &dwlen );
        if(bRet==TRUE) {
        DBMS4( stdout, "DeviceInstanceId = %s\n", buf);
        }
        // COM�|�[�g�ԍ����擾
        hKey = SetupDiOpenDevRegKey( hDevI, &DeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE );
        if( NULL !=hKey ){
            DWORD tmp_type = 0;
	    char comname[256];
            dwlen = sizeof( comname );

	    RegQueryValueEx( hKey, _T("PortName"), NULL, &tmp_type , (LPBYTE)comname, &dwlen );
            RegCloseKey(hKey);
            hKey = NULL;
            DBMS4( stdout, "DeviceName = %s\n", buf);

	    {
	        ULONG nStatus;              // DN_ �ړ��q�̒萔
		ULONG nProblemNumber = 0;   // CM_PROB_ �ړ��q�̒萔
		CONFIGRET cgr = CM_Get_DevNode_Status( &nStatus, &nProblemNumber, DeviceInfoData.DevInst, 0 );	
		if( cgr == CR_SUCCESS && nStatus & DN_DISABLEABLE ) {
		    /* �������\�Ŗ���������Ă���ꍇ�́A���X�g����͂��� */
		    if(nProblemNumber == CM_PROB_DISABLED) {
		        DBMS4( stdout, "is_disabled\n");
			continue;
		    }
		}
	    }

	    if(!_strnicmp( comname, "COM", 3)) {
		/* �|�[�g���̒ǉ������@*/
		    s3::mcxx_dev232_table_t *t;
		void *const old_ptr = (void*)l;
		size = sizeof(s3::mcxx_dev232_list_t) + (sizeof(s3::mcxx_dev232_table_t) * (1 + l->num_tabs));
		
		l = (s3::mcxx_dev232_list_t*)realloc( old_ptr, size);
		if( NULL == l ) {
		    free(old_ptr);
		    status = ENOMEM;
		    goto out;
		}
		t =&l->tabs[l->num_tabs];
		memset( t, 0x0, sizeof(s3::mcxx_dev232_table_t));
		strcpy(t->path, comname);
		++l->num_tabs;
	    }
        }
    }

    if(l->num_tabs == 0 ) {
	/* �������j�� */
	status = ENODEV;
    } else {
	status = 0;
    }

out:

    if( NULL != hKey ) {
        RegCloseKey(hKey);
    }

    if( NULL != hDevI ) {
	SetupDiDestroyDeviceInfoList(hDevI);
    }

    if(status) {
	if(l) {
	    free(l);
	    l = NULL;
	}
	errno = status;
    }

     return (status == 0 ) ? l : NULL;
}

#endif

void  s3::mcxx_dev232_delete_devlist( s3::mcxx_dev232_list_t *const l )
{
    if( NULL != l ) {
	free(l);
    }

    return;
}

typedef struct _dev232_handle_ext {
    HANDLE h_comport;
    uint32_t timeout_msec;
} dev232_handle_ext_t;

#define get_dev232_handle_ext(s) MPXX_STATIC_CAST(dev232_handle_ext_t *,(s)->ext)

int s3::mcxx_dev232_open( s3::mcxx_dev232_handle_t *const self_p, const char *const path)
{
    dev232_handle_ext_t *e;
    int status;
    BOOL bRet;

    memset( self_p, 0x0, sizeof(dev232_handle_ext_t));
    e = (dev232_handle_ext_t*)malloc(sizeof(dev232_handle_ext_t));
    if( NULL == e ) {
	return ENOMEM;
    }
    memset( e, 0x0, sizeof(dev232_handle_ext_t));
    self_p->ext = e; 

    /* �f�o�C�X�I�[�v�� */
    e->h_comport = CreateFile(_T(path), GENERIC_READ | GENERIC_WRITE, 0, NULL,
	OPEN_EXISTING, FILE_FLAG_OVERLAPPED /* || FILE_ATTRIBUTE_NORMAL */, NULL);
    if( INVALID_HANDLE_VALUE == e->h_comport ) {
	status = ENODEV;
	goto out;
    }

    /* ����M�o�b�t�@�̃T�C�Y���� */
    bRet = SetupComm( e->h_comport,
	//   �o�b�t�@�[�T�C�Y�F�@��M�̃o�b�t�@�[�T�C�Y���o�C�g�P�ʂŎw��( YMODEM:1200. EtherNet:1600)
	1600, 1600);
    if( FALSE == bRet ) {
	DBMS1( stderr, __FUNCTION__ " : SetupComm fail\n");
	status = EACCES;
	goto out;
    }

    /* ����M�o�b�t�@������ */
    bRet = PurgeComm(e->h_comport,
	//   ���s���鑀��F ��L�͖������̓Ǐ����̒��~�y�ё���M�̃o�b�t�@�[�̃N���A���w��
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
	s3::mcxx_dev232_close(self_p);
    }

    return status;
}

void s3::mcxx_dev232_close( s3::mcxx_dev232_handle_t *self_p)
{
    dev232_handle_ext_t * const e = get_dev232_handle_ext(self_p);
    if( NULL != e->h_comport ) {
	CloseHandle(e->h_comport);
    }

    free(self_p->ext);
    self_p->ext = NULL;

    return;
}

static int flowctrl_attr2wincom( DCB *dcb_p, const s3::enum_mcxx_dev232_flowtype_t flowtype)
{
    switch(flowtype) {
    case s3::MCXX_DEV232_FLOW_OFF:
	dcb_p->fRtsControl = RTS_CONTROL_DISABLE;
        dcb_p->fOutxCtsFlow = FALSE;
        dcb_p->fInX = FALSE;
        dcb_p->fOutX = FALSE;
	break;
    case s3::MCXX_DEV232_FLOW_HW:
	dcb_p->fRtsControl = RTS_CONTROL_HANDSHAKE;
        dcb_p->fOutxCtsFlow = TRUE;
        dcb_p->fInX = FALSE;
        dcb_p->fOutX = FALSE;
	break;
    case s3::MCXX_DEV232_FLOW_XONOFF:
	dcb_p->fRtsControl = RTS_CONTROL_DISABLE;
        dcb_p->fOutxCtsFlow = FALSE;
        dcb_p->fInX = TRUE;
        dcb_p->fOutX = TRUE;
	break;
    case s3::MCXX_DEV232_FLOW_unknown:
    case s3::MCXX_DEV232_FLOW_eod:
    default:
	return -1;
    }
    return 0;
}

static s3::enum_mcxx_dev232_flowtype_t flowctrl_wincom2attr( DCB *dcb_p )
{
    if( (dcb_p->fInX == FALSE) && (dcb_p->fOutX == FALSE ) ) {
	if( dcb_p->fRtsControl == RTS_CONTROL_DISABLE ) {
	    if( dcb_p->fOutxCtsFlow == FALSE ) {
		return s3::MCXX_DEV232_FLOW_OFF;
	    }
	} else if( dcb_p->fRtsControl == RTS_CONTROL_HANDSHAKE ) {
	    if(dcb_p->fOutxCtsFlow == TRUE ) {
		return s3::MCXX_DEV232_FLOW_HW;
	    }
	} 
    } else if( (dcb_p->fInX == TRUE) && (dcb_p->fOutX == TRUE ) ) {
	if( ( dcb_p->fRtsControl == RTS_CONTROL_DISABLE ) && ( dcb_p->fOutxCtsFlow == FALSE )) {
	    return s3::MCXX_DEV232_FLOW_XONOFF;
	}
    } 
    return s3::MCXX_DEV232_FLOW_unknown;
} 
 
static s3::enum_mcxx_dev232_databitstype_t datebits_com2attr(const BYTE bytesize)
{
    switch(bytesize) {
    case 5:
	return s3::MCXX_DEV232_DATA_5;
    case 6:
	return s3::MCXX_DEV232_DATA_6;
    case 7:
	return s3::MCXX_DEV232_DATA_7;
    case 8:
	return s3::MCXX_DEV232_DATA_8;
    }
    return s3::MCXX_DEV232_DATA_unknown;
}

static BYTE databits_attr2com(const s3::enum_mcxx_dev232_databitstype_t bytesize)
{
    switch(bytesize) {
    case s3::MCXX_DEV232_DATA_5:
	return 5;
    case s3::MCXX_DEV232_DATA_6:
	return 6;
    case s3::MCXX_DEV232_DATA_7:
	return 7;
    case s3::MCXX_DEV232_DATA_8:
	return 8;
    default:
	return ~0;
    }
}

static s3::enum_mcxx_dev232_stopbitstype_t stopbits_com2attr( BYTE StopBits)
{
    switch(StopBits) {
    case ONESTOPBIT: 
	return s3::MCXX_DEV232_STOP_1;
    case TWOSTOPBITS: 
	return s3::MCXX_DEV232_STOP_2;
    default:
	return s3::MCXX_DEV232_STOP_unknown;
    }
}

static BYTE stopbits_attr2com( const s3::enum_mcxx_dev232_stopbitstype_t StopBits) 
{
    switch(StopBits) {
    case s3::MCXX_DEV232_STOP_1:
	return ONESTOPBIT;
    case s3::MCXX_DEV232_STOP_2:
	return TWOSTOPBITS;
    default:
	return ~0;
    }
}

static s3::enum_mcxx_dev232_paritytype_t parity_com2attr( BYTE Parity)
{
    switch(Parity) {
    case NOPARITY:
	return  s3::MCXX_DEV232_PAR_NONE;
    case ODDPARITY:
	return  s3::MCXX_DEV232_PAR_ODD;
    case EVENPARITY:
	return  s3::MCXX_DEV232_PAR_EVEN;
    default:
	return  s3::MCXX_DEV232_PAR_unknown;
    }
}

static BYTE parity_attr2com(s3::enum_mcxx_dev232_paritytype_t Parity)
{
    switch(Parity) {
    case s3::MCXX_DEV232_PAR_NONE:
	return NOPARITY;
    case s3::MCXX_DEV232_PAR_ODD:
	return ODDPARITY;
    case s3::MCXX_DEV232_PAR_EVEN:
	return EVENPARITY;
    default:
	return ~0;
    }
}

/**
 * @fn int s3::mcxx_dev232_get_attr( s3::mcxx_dev232_handle_t *self_p, s3::mcxx_dev232_attr_t *attr_p)
 * @brief ���݂�EIA232�|�[�g�̑������擾���܂��B
 * @retva 0 ����
 * @retval -1 ���s(������)
 **/
int s3::mcxx_dev232_get_attr( s3::mcxx_dev232_handle_t *const self_p, s3::mcxx_dev232_attr_t *const attr_p)
{
    dev232_handle_ext_t * const e = get_dev232_handle_ext(self_p);
    DCB dcb;
    
    GetCommState( e->h_comport, &dcb);
    memset( attr_p, 0x0, sizeof(s3::mcxx_dev232_attr_t));

    attr_p->baudrate = dcb.BaudRate;
    attr_p->databits = datebits_com2attr(dcb.ByteSize);
    attr_p->stopbits = stopbits_com2attr(dcb.StopBits);
    attr_p->parity   = parity_com2attr(dcb.Parity);
    attr_p->flowctrl = flowctrl_wincom2attr(&dcb);
    attr_p->timeout_msec = e->timeout_msec;

    return 0;
}

/**
 * @fn int s3::mcxx_dev232_set_attr( s3::mcxx_dev232_handle_t *self_p, const s3::mcxx_dev232_attr_t *attr_p)
 * @biref EIA232�|�[�g�̑�����ݒ肵�܂��B
 * @retva 0 ����
 * @retval -1 ���s(������)
 */
int s3::mcxx_dev232_set_attr( s3::mcxx_dev232_handle_t *const self_p, const s3::mcxx_dev232_attr_t *const attr_p)
{
    dev232_handle_ext_t * const e = get_dev232_handle_ext(self_p);
    DCB dcb;

    GetCommState( e->h_comport, &dcb);

    dcb.BaudRate = attr_p->baudrate;
    dcb.ByteSize = databits_attr2com(attr_p->databits);
    dcb.StopBits = stopbits_attr2com(attr_p->stopbits);
    dcb.Parity   = parity_attr2com(attr_p->parity);
    dcb.fParity  = ( attr_p->parity != s3::MCXX_DEV232_PAR_NONE ) ? TRUE : FALSE;
    
    flowctrl_attr2wincom( &dcb, attr_p->flowctrl);
    e->timeout_msec = attr_p->timeout_msec;

    SetCommState( e->h_comport, &dcb);

    return 0;
}

/**
 * @fn int s3::mcxx_dev232_read_stream( s3::mcxx_dev232_handle_t *const self_p, void *const buf, const size_t length, size_t *const rlen_p)
 * @brief �o�C�i���X�g���[����ǂݍ��݂܂��B
 * @retval 0 ����
 * @retval EIO ���z���x����IO�G���[����������
 * @retval ETIMEDOUT ���삪�^�C���A�E�g����
 **/
int s3::mcxx_dev232_read_stream( s3::mcxx_dev232_handle_t *const self_p, void *const buf, const size_t length, size_t *const rlen_p)
{
    dev232_handle_ext_t * const e = get_dev232_handle_ext(self_p);

    return s3::mcxx_dev232_read_stream_withtimeout( self_p, buf, length, rlen_p, e->timeout_msec);
}

/**
 * @fn int s3::mcxx_dev232_read_stream_withtimeout( s3::mcxx_dev232_handle_t *const self_p, void *const buf, const int length, size_t *const rlen_p, const int timeout_msec)
 * @brief �o�C�i���X�g���[����ǂݍ��ݑO�^�C���A�E�g�t���œǂݍ��݂܂��B
 * @retval 0 ����
 * @retval EIO ���z���x����IO�G���[����������
 * @retval ETIMEDOUT ���삪�^�C���A�E�g����
 **/
int s3::mcxx_dev232_read_stream_withtimeout( s3::mcxx_dev232_handle_t *const self_p, void *const buf, const int length, size_t *const rlen_p, const int timeout_msec)
{
    dev232_handle_ext_t * const e = get_dev232_handle_ext(self_p);

    int status;
    DWORD dwBytes, dwResult;
    DWORD bRet;
    const DWORD dwTimeOutVal = (timeout_msec == 0) ? INFINITE : timeout_msec;
    size_t rlen = 0, leftlen = length;
    OVERLAPPED  olReader = {0};             // �񓯊��ǂݍ��ݗp�\����

    if( NULL == self_p ) {
	return EINVAL;
    }

    olReader.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    if( olReader.hEvent == NULL ) {
	/* �C�x���g����p�n���h���������s */
	return EIO;
    }

    // �񓯊��ǂݍ��݊J�n
    bRet = ReadFile( e->h_comport, buf, length, &dwBytes, &olReader );

    // �ǂݍ��݌��ʃ`�F�b�N
    // �P�o�C�g�ǂݍ��߂��TRUE���A�ǂݍ��݊����҂��܂��̓G���[�̎���
    // FALSE���Ԃ�܂��B
    if( bRet == FALSE ) {
        // �ǂݍ��݃G���[
        // �ǂݍ��݊����҂��̎���GetLastError()�֐��̖߂��ERROR_IO_PENDING��
        // �Ԃ���܂��B����ȊO�̏ꍇ�͖{���ɃG���[�ł��B
        if( GetLastError() != ERROR_IO_PENDING ) {
	    return EIO;
	} 
    } else {
        // �ǂݍ��݂n�j
	status = 0;
	goto out;
    }

    // �񓯊��ǂݍ��݊����҂�����
    dwResult = WaitForSingleObject( olReader.hEvent, dwTimeOutVal);
    switch (dwResult) {
	case WAIT_OBJECT_0:
	    if( !GetOverlappedResult( e->h_comport, &olReader, &dwBytes, FALSE )) {
		/* ����G���[ */
		status = EIO;
	    } else {
		status = 0;
	    }
	    break;
	case WAIT_ABANDONED:
	    status = EINVAL;
	    break;
	case WAIT_TIMEOUT:
	    status = ETIMEDOUT;
	    break;
	default:
	    status = EIO;
    }

out:

    CloseHandle(olReader.hEvent);

    if(NULL != rlen_p) {
	*rlen_p = (size_t)dwBytes;
    }

    return status;
}

/**
 * @fn int s3::mcxx_dev232_clear_recv( s3::mcxx_dev232_handle_t *const self_p)
 * @brief ��M�o�b�t�@�̃f�[�^��r�����܂�
 * @retval EIO �Ƃ肠�����G���[����������
 **/
int s3::mcxx_dev232_clear_recv( s3::mcxx_dev232_handle_t *const self_p)
{
    const int TRYOUT = 10;
    dev232_handle_ext_t * const e = get_dev232_handle_ext(self_p);
 
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
	   dwCount = ComStat.cbInQue;   // �L���[���̃f�[�^�����Q��
    	} while (dwCount == 0 && (trycnt != TRYOUT));

	if(trycnt == TRYOUT ) {
	    return 0;
	}
	if(!(dwCount < 1024)) {
		dwCount = 1024;
	}

	result = s3::mcxx_dev232_read_stream( self_p, &buf, dwCount, &rlen);
	if(result) {
	    return EIO;
	}
	dwCount -= rlen;

    }

    return 0;
}



/**
 * @fn int s3::mcxx_dev232_readln_string( s3::mcxx_dev232_handle_t *const self_p, char *buf, const size_t bufsize, size_t *const recvlen_p)
 * @brief NULL�����܂��͉��s�����܂ł���M���܂��B
 *	�� ���s�����͂܂���������Ă��܂���B
 * @retval 0 ����
 * @retval EIO �Ƃ肠�����G���[����������
 */
int s3::mcxx_dev232_readln_string( s3::mcxx_dev232_handle_t *const self_p, char *buf, const size_t bufsize, size_t *const recvlen_p)
{
    int status;
    dev232_handle_ext_t * const e = get_dev232_handle_ext(self_p);
//     DWORD dwCount;
    int result;
    size_t recvlen =0, recv;
    char *pbuf=(char*)buf;

    while(1) {
#if 0
	do {
	    COMSTAT ComStat;
	    DWORD dwErr;
	    ClearCommError( e->h_comport, &dwErr, &ComStat);
	    dwCount = ComStat.cbInQue;   // �L���[���̃f�[�^�����Q��
    	} while (dwCount == 0 );
#endif

	result = s3::mcxx_dev232_read_stream( self_p, pbuf, 1, &recv);
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

const char *s3::mcxx_dev232_databitstype2str(s3::enum_mcxx_dev232_databitstype_t databits)
{
/* *INDENT-OFF* */
    struct {
	s3::enum_mcxx_dev232_databitstype_t id;
	const char *const tag;
    } tabs[] = {
	{ s3::MCXX_DEV232_DATA_5, "5bits"},
	{ s3::MCXX_DEV232_DATA_6, "6bits"},
	{ s3::MCXX_DEV232_DATA_7, "7bits"},
	{ s3::MCXX_DEV232_DATA_8, "8bits"},
	{ s3::MCXX_DEV232_DATA_unknown, "unknown"},
	{ s3::MCXX_DEV232_DATA_eod, NULL}
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

const char *s3::mcxx_dev232_paritytype2str(s3::enum_mcxx_dev232_paritytype_t paritytype)
{
/* *INDENT-OFF* */
    struct {
	s3::enum_mcxx_dev232_paritytype_t id;
	const char *const tag;
    } tabs[] = {
	{ s3::MCXX_DEV232_PAR_NONE   , "NONE"},
	{ s3::MCXX_DEV232_PAR_ODD    , "ODD"},
	{ s3::MCXX_DEV232_PAR_EVEN   , "EVEN"},
	{ s3::MCXX_DEV232_PAR_unknown, "unknown"},
	{ s3::MCXX_DEV232_PAR_eod    , NULL}
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

const char *s3::mcxx_dev232_stopbitstype2str(s3::enum_mcxx_dev232_stopbitstype_t stopbits) {
/* *INDENT-OFF* */
    struct {
	s3::enum_mcxx_dev232_stopbitstype_t id;
	const char *const tag;
    } tabs[] = {
	{ s3::MCXX_DEV232_STOP_1, "1bit"},
	{ s3::MCXX_DEV232_STOP_2, "2bit"},
	{ s3::MCXX_DEV232_STOP_unknown, "unknown"},
	{ s3::MCXX_DEV232_STOP_eod   , NULL}
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

const char *s3::mcxx_dev232_flowtype2str(s3::enum_mcxx_dev232_flowtype_t flowtype) {
/* *INDENT-OFF* */
    struct {
	s3::enum_mcxx_dev232_flowtype_t id;
	const char *const tag;
    } tabs[] = {
	{ s3::MCXX_DEV232_FLOW_OFF, "OFF"},
	{ s3::MCXX_DEV232_FLOW_HW , "HW"},
	{ s3::MCXX_DEV232_FLOW_XONOFF, "XONOFF"},
	{ s3::MCXX_DEV232_FLOW_unknown, "unknown"},
	{ s3::MCXX_DEV232_FLOW_eod    , NULL}
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

/**
 * @fn int s3::mcxx_dev232_write_stream_withtimeout( s3::mcxx_dev232_handle_t *const self_p, const void *dat, const size_t sendlen, int *const rlen_p, timeout_msec)
 * @brief �o�C�i���X�g���[����ǂݍ��ݑO�^�C���A�E�g�t���œǂݍ��݂܂��B
 *    �@�^�C���A�E�g�l�͑����w�肳�ꂽ���̂��g���܂�
 * @retval 0 ����
 * @retval EIO ���z���x����IO�G���[����������
 * @retval ETIMEDOUT ���삪�^�C���A�E�g����
 **/
int s3::mcxx_dev232_write_stream( s3::mcxx_dev232_handle_t *const self_p, const void *dat, const size_t sendlen)
{
    dev232_handle_ext_t * const e = get_dev232_handle_ext(self_p);

    return s3::mcxx_dev232_write_stream_withtimeout(self_p,dat, sendlen, NULL, e->timeout_msec);
}

/**
 * @fn int s3::mcxx_dev232_write_stream_withtimeout( s3::mcxx_dev232_handle_t *const self_p, const void *dat, const size_t sendlen, size_t *const rlen_p, int timeout_msec)
 * @brief �o�C�i���X�g���[����ǂݍ��ݑO�^�C���A�E�g�t���œǂݍ��݂܂��B
 *    ���Ӂj�t���[����������ꍇ�̂݃^�C���A�E�g����͂�
 * @retval 0 ����
 * @retval EIO ���z���x����IO�G���[����������
 * @retval ETIMEDOUT ���삪�^�C���A�E�g����
 **/
int s3::mcxx_dev232_write_stream_withtimeout( s3::mcxx_dev232_handle_t *const self_p, const void *dat, const size_t sendlen, size_t *const rlen_p, int timeout_msec)
{
    dev232_handle_ext_t * const e = get_dev232_handle_ext(self_p);
    int status;
    DWORD dwBytes, dwResult;
    DWORD bRet;
    const DWORD dwTimeOutVal = (timeout_msec == 0) ? INFINITE : timeout_msec;
    size_t rlen = 0;
    OVERLAPPED  olWriter = {0};             // �񓯊��ǂݍ��ݗp�\����

    if( NULL == self_p ) {
	return EINVAL;
    }

    olWriter.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    if( olWriter.hEvent == NULL ) {
	/* �C�x���g����p�n���h���������s */
	return EIO;
    }

    // �񓯊��ǂݍ��݊J�n
    bRet = WriteFile( e->h_comport, dat, sendlen, &dwBytes, &olWriter );

    // �������݌��ʃ`�F�b�N
    // ��������TRUE���A�ǂݍ��݊����҂��܂��̓G���[�̎���
    // FALSE���Ԃ�܂��B
    if( bRet == FALSE ) {
        // �������݃G���[
        // �����҂��̎���GetLastError()�֐��̖߂��ERROR_IO_PENDING��
        // �Ԃ���܂��B����ȊO�̏ꍇ�͖{���ɃG���[�ł��B
        if( GetLastError() != ERROR_IO_PENDING ) {
	    return EIO;
	} 
    } else {
        // �ǂݍ��݂n�j
	status = 0;
	goto out;
    }

    // �񓯊��������݊����҂������i�o�b�t�@�������O�ɂȂ�܂ł܂��Ă܂�)
    dwResult = WaitForSingleObject( olWriter.hEvent, dwTimeOutVal);
    switch (dwResult) {
	case WAIT_OBJECT_0:
	    if( !GetOverlappedResult( e->h_comport, &olWriter, &dwBytes, FALSE )) {
		/* ����G���[ */
		status = EIO;
	    } else {
		status = 0;
	    }
	    break;
	case WAIT_ABANDONED:
	    status = EINVAL;
	    break;
	case WAIT_TIMEOUT:
	    if( !GetOverlappedResult( e->h_comport, &olWriter, &dwBytes, FALSE )) {
		/* ����G���[ */
		status = EIO;
	    } else {
		status = ETIMEDOUT;
	    }
	    break;
	default:
	    status = EIO;
    }

out:

    CloseHandle(olWriter.hEvent);

    if(NULL != rlen_p) {
	*rlen_p = (size_t)dwBytes;
    }

    return status;

}


/**
 * @fn int s3::mcxx_dev232_write_string( s3::mcxx_dev232_handle_t *const self_p, const char *const str, size_t *const retlen_p )
 * @brief ������𑗐M����
 * @param self_p s3::mcxx_dev232_handle_t�C���X�^���X�\���̃|�C���^
 * @param retlen_p ���M�ς݃T�C�Y�i�I�[�����܂�)
 * @retval 0 ����
 * @retvao EIO I/O���s
 **/
int s3::mcxx_dev232_write_string( s3::mcxx_dev232_handle_t *const self_p, const char *const str, size_t *const retlen_p )
{
    int result;
    const size_t sendlen = strlen(str) + 1;

    result = s3::mcxx_dev232_write_stream(self_p, (void*)str, sendlen);
    if(result) {
	return result;
    }

    if(NULL != retlen_p) {
	*retlen_p = sendlen;
    }

    return result;
}

/**
 * @fn int s3::mcxx_dev232_string_command_execute( s3::mcxx_dev232_handle_t *const self_p, const char *send_cmd, char *recv_str, const size_t szofrecv)
 * @brief ������R�}���h�𑗐M����
 * @param self_p s3::mcxx_dev232_handle_t�C���X�^���X�\���̃|�C���^
 * @param send_cmd ���M�R�}���h������|�C���^
 * @param recv_str ��M������擾�o�b�t�@
 * @param szofresv ��M�o�b�t�@�T�C�Y
 **/
int s3::mcxx_dev232_string_command_execute( s3::mcxx_dev232_handle_t *const self_p, const char *send_cmd, char *recv_str, const size_t bufszofrecv)
{
    int result;
    size_t retlen;

    result = s3::mcxx_dev232_write_string( self_p, send_cmd, &retlen);
    if(result) {
	return result;
    }

    result = s3::mcxx_dev232_readln_string( self_p, recv_str, bufszofrecv, &retlen);
    if(result) { 
	return result;
    }

    return 0;
}
