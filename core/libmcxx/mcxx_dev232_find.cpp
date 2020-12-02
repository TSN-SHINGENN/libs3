/**
 *	Copyright 2013 TSN-SHINGENN
 *
 *	Basic Author: Seiichi Takeda  '2013-March-07 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @fn eia232_find.c
 * @brief �x���_ID�A�v���_�N�gID����Ή�����V���A���C���^�[�t�F�[�X������������N���X
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
#include <string.h>
#include <stdint.h>

/* libs3 */
#include <libmpxx/mpxx_sys_types.h>
#include <libmpxx/mpxx_string.h>
#include <libmpxx/mpxx_cl2av.h>

#ifdef DEBUG
static int debuglevel = 4;
#else
static int debuglevel = 4;
#endif
#include <libmpxx/dbms.h>

/* this */
#include "mcxx_dev232.h"
#include "mcxx_dev232_find.h"

#if defined(WIN32)
#include <tchar.h>
#include <windows.h>
#include <setupapi.h>
#pragma comment(lib, "setupapi.lib")
#endif

typedef struct _win32_find_device_ext {
    size_t num_ports;
} win32_find_device_ext_t;

#define get_find_device_ext(s) MPXX_STATIC_CAST( win32_find_device_ext_t*, (s)->ext)

/**
 * @fn int s3::mcxx_dev232_find_device_init( s3::mcxx_dev232_find_device_t *self_p)
 * @brief s3::mcxx_dev232_find_device_t�C���X�^���X�����������܂��B
 * @param self_p s3::mcxx_dev232_find_device_t�C���X�^���X�|�C���^
 * @retval 0 ����
 * @retval ENOMEM ���\�[�X���l���ł��Ȃ�����
 * @retval ����ȊO �G���[�����Ɩ���`
 */
int s3::mcxx_dev232_find_device_init( s3::mcxx_dev232_find_device_t *const self_p)
{
    win32_find_device_ext_t *e;

    memset(self_p, 0x0, sizeof(s3::mcxx_dev232_find_device_t));

    e = (win32_find_device_ext_t*)malloc(sizeof(win32_find_device_ext_t));
    if( NULL == e ) {
	return ENOMEM;
    }
    memset( e, 0x0, sizeof(win32_find_device_ext_t));

    e->num_ports = ~0;
    self_p->ext = (void*)e;

    return 0;
}

/**
 * @fn int s3::mcxx_dev232_find_device_reset_instance(s3::mcxx_dev232_find_device_t *const self_p)
 * @brief s3::mcxx_dev232_find_device_t�C���X�^���X�̃f�o�C�X�����ʒu��擪�Ɉړ����܂�
 * @param self_p s3::mcxx_dev232_find_device_t�C���X�^���X�|�C���^
 * @retval 0 ����
 * @retval ����ȊO �G���[�����Ɩ���`
 */
int s3::mcxx_dev232_find_device_reset_instance(s3::mcxx_dev232_find_device_t *const self_p)
{
    win32_find_device_ext_t *const e = get_find_device_ext(self_p);

    e->num_ports = ~0;
    self_p->index_point = 0;
    self_p->num_found_device = 0;

    return 0;
}

/**
 * @fn int s3::mcxx_dev232_find_device_exec( s3::mcxx_dev232_find_device_t *const self_p, const uint32_t vid,  const uint32_t pid, const char *const name_buf, const size_t bufsize)
 * @brief �w�肳�ꂽ�V���A���|�[�g���������܂��B(�C���X�^���X���������PnP���������ꂽ�ꍇ�A���܂��F���ł��Ȃ����Ƃ�����܂��B)
 * @param self_p s3::mcxx_dev232_find_device_t�C���X�^���X�|�C���^
 * @param vid �x���_�[ID�ԍ�
 * @param pid �v���_�N�gID�ԍ�
 * @param name_buf �f�o�C�X�t�@�C�����o�^�o�b�t�@�|�C���^
 * @param bufsize name_buf�̃o�b�t�@�T�C�Y
 * @retval 0 ������
 * @retval ENOMEM bufsize���K�v�T�C�Y�܂ő���Ȃ�����
 * @retval ENODEV ������Ȃ��i���o�I���_�ɓ��B)
 * @retval EIO �V�X�e���G���[����������(���̌�̌��������͕s����ɂȂ�)
 */
int s3::mcxx_dev232_find_device_exec( s3::mcxx_dev232_find_device_t *const self_p, const uint32_t vid,  const uint32_t pid, char *const name_buf, const size_t bufsize)
{
    win32_find_device_ext_t *const e = get_find_device_ext(self_p);
    HKEY hKey = NULL;
    HDEVINFO hDevI = NULL;
    DWORD dwlen;
    size_t n;
    int status, result;
    char buf[512];

    if( e->num_ports == ~0 ) {
	/* WIN32�̏ꍇ */
	LONG lresult;
	DWORD dwnum_ports;

	/* �V���A���|�[�g�Ƃ��đ��݂��郌�W�X�g���L�[���J�� */
	lresult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("HARDWARE\\DEVICEMAP\\SERIALCOMM"), 0, KEY_READ, &hKey );
	if( lresult != ERROR_SUCCESS ) {
	    status = ENODEV;
	    goto out;
	}

	/**
	 * @note �v�f�����擾(�f�o�C�X�łȂ����̂��܂܂��
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

    // ���W�X�g���ɓo�^����Ă���COM�|�[�g�̃f�o�C�X�����擾
    hDevI = SetupDiGetClassDevs( &GUID_DEVINTERFACE_COMPORT, NULL, NULL, (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));
    if( NULL == hDevI ) {
        status = EIO;
        goto out;
    }

    for( n=self_p->index_point; n<e->num_ports; ++n) {
        BOOL bRet;
	SP_DEVINFO_DATA DeviceInfoData = { sizeof(SP_DEVINFO_DATA)};
        bRet = SetupDiEnumDeviceInfo( hDevI, (DWORD)n, &DeviceInfoData );
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

        bRet = SetupDiGetDeviceRegistryProperty( hDevI, &DeviceInfoData, SPDRP_HARDWAREID, NULL, (PBYTE)buf,sizeof(buf),&dwlen );
        if(bRet!=TRUE) {
	    return EIO;
        }

	if(!mpxx_strncasecmp( buf, "ACPI", 4)) {
	    /* �I���{�[�h�f�o�C�X */
	    continue;
	}

	if(!mpxx_strncasecmp( buf, "USB", 3)) {
	    mpxx_cl2av_t marg;
	    size_t c;
	    uint32_t dvid, dpid;
	    dvid = dpid = ~0;

	    /* USB�f�o�C�X */
	    result = mpxx_cl2av_init( &marg,"\\&_", '\0', NULL);
	    if(result) {
		return EIO;
	    }
	    result = mpxx_cl2av_execute( &marg, buf);
	    if(result) {
		return EIO;
	    }
	    for( c=0; c<marg.argc; ++c) {
		if(!mpxx_strcasecmp( marg.argv[c], "VID")) {
		    if( sscanf( marg.argv[c+1], "%x", &dvid) != 1 ) {
			return EIO;
		    }
		    ++c;
		    continue;
		}
		if(!mpxx_strcasecmp( marg.argv[c], "PID")) {
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
	    mpxx_cl2av_destroy(&marg);
	    if( (vid == dvid) && ( pid == dpid ) ) {
		/* found */
	        // �f�o�C�X�C���X�^���XID���擾
	        bRet = SetupDiGetDeviceInstanceId( hDevI, &DeviceInfoData, buf, sizeof(buf), &dwlen );
		if(bRet==TRUE) {
		    DBMS4( stdout, "DeviceInstanceId = %s\n", buf);
		}
		// COM�|�[�g�ԍ����擾
		hKey = SetupDiOpenDevRegKey( hDevI, &DeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE );
		if( NULL !=hKey ){
		    DWORD tmp_type = 0;
		    dwlen = sizeof( buf );
		    RegQueryValueEx( hKey, _T("PortName"), NULL, &tmp_type , (PBYTE)buf, &dwlen );
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
	/* �Ō�܂œ��B���� */
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
 * @fn void s3::mcxx_dev232_find_device_destroy( s3::mcxx_dev232_find_device_t *self_p)
 * @brief s3::mcxx_dev232_find_device_t�C���X�^���X��j�����܂��B
 */
void s3::mcxx_dev232_find_device_destroy( s3::mcxx_dev232_find_device_t *self_p)
{
    if( NULL != self_p ) {
	free(self_p->ext);
	self_p->ext = NULL;
    }

    return;
}

