/**
 *      Copyright 2004 TSN-SHINGENN All Rights Reserved.
 *
 *      Basic Author: Seiichi Takeda  '2004-March-26 Active
 *              Last Alteration $Author: takeda $
 */

/**
 * @file mpsx_unistd.c
 * @brief POSIX unistdライブラリのラッパーライブラリコード
 */

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef WIN32
/* Microsoft Windows Series */
#if _MSC_VER >= 1400		/* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif

#if defined(WINVER)
#undef WINVER
#endif
#if defined(_WIN32_WINNT)
#undef _WIN32_WINNT
#endif

#define _WIN32_WINNT 0x0501	/* このソースコードは WindowsXP以上を対象にします。 */
#define WINVER 0x0501		/*  ↑ */

#include <windows.h>
#else
/* Unix like OS */
#include <stdbool.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>
#include <stdbool.h>
#endif

#if defined(Linux)
#include <pwd.h>
#endif

#if defined(Darwin)
#include <mach/host_info.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#endif

#if defined(QNX)
#include <sys/stat.h>
#include <sys/neutrino.h>
#include <sys/syspage.h>
#if _NTO_VERSION < 632
#include <sys/param.h>
#include <sys/sysctl.h>
#endif
#endif

#if defined(__AVM2__)
#include <AS3/AS3.h>
#endif

#if defined(XILKERNEL) && !defined(STANDALONE)
#include <sys/process.h>
#elif defined(_LIBMBMCS)
//#include <libmbmcs/libmbmcs_libmultios_bridge.h>
#include "libmbmcs_libmultios_bridge.h"
#endif

#ifdef __GNUC__
__attribute__ ((unused))
#endif
#ifdef DEBUG
static int debuglevel = 1;
#else
static const int debuglevel = 0;
#endif
#include "dbms.h"

#if defined(_S3_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

#include "mpsx_malloc.h"
#include "mpsx_string.h"
#include "mpsx_unistd.h"

#if defined(_S3_MICROCONTROLLER_ENABLE_GCC_OPTIMIZE_FOR_SIZE )

extern unsigned int mpsx_sleep( const unsigned int )
		__attribute__ ((optimize("Os")));
extern void mpsx_msleep( const unsigned int )
		__attribute__ ((optimize("Os")));
extern int64_t mpsx_sysconf( const enum_mpsx_sysconf_t name )
		__attribute__ ((optimize("Os")));
extern int mpsx_is_user_an_admin(void)
		__attribute__ ((optimize("Os")));
extern int mpsx_get_username( char*, const size_t)
		__attribute__ ((optimize("Os")));
extern int mpsx_get_hostname( char*, const size_t)
		__attribute__ ((optimize("Os")));
int mpsx_fork(void)
		__attribute__ ((optimize("Os")));

#endif /* end of _S3_MICROCONTROLLER_ENABLE_GCC_OPTIMIZE_FOR_SIZE */

/**
 * @note Xilkernelでsleep()関数を使う場合はBSPセッティングでconfig_timeをtrueにすること
 **/

/**
 * @fn unsigned int mpsx_sleep(const unsigned int seconds)
 * @brief 指定秒間休止する。
 *	シグナルによって割り込まれることがあります。
 * @param seconds 秒
 * @return とりあえず0固定
 */
unsigned int mpsx_sleep(const unsigned int seconds)
{

    if (!seconds) {
#if defined(_LIBMBMCS)
	return 0;
#elif defined(XILKERNEL) && defined(STANDALONE)
	DMSG( stderr, "not supported %s(like sched_yield) function"EOL_CRLF, __func__);
	abort();
#elif defined(XILKERNEL) && !defined(STANDALONE)
#if defined(CONFIG_YIELD)
	yield();
#else
	sleep(0);
#endif /* end of CONFIG_YIELD */
#elif defined(WIN32)
	Sleep(0);
#else /* posix */
	sched_yield();
#endif
	return 0;
    }
#if defined(WIN32)
    Sleep(seconds * 1000);
    return seconds;
#elif defined(_LIBMBMCS)
    return mbmcs_sleep(seconds);
#elif defined(XILKERNEL) && defined(STANDALONE)
	DMSG( stderr, "not supported %s() function"EOL_CRLF, __func__);
	abort();
#elif  defined(XILKERNEL) && !defined(STANDALONE)
    sleep( seconds * 1000);
#elif defined(Linux) || defined(QNX) || defined(Darwin) || defined(__AVM2__)
    /* POSIX */
    /**
     * @note POSIXではsleep()を使いたいが シグナルやalarm()を使ったときに問題が発生するためnanosleepで実装する。
     */
#if 0 /* Legacy */
    return sleep(seconds);
#else  /* new */
    {
	struct timespec req;
	req.tv_sec = seconds;
	req.tv_nsec = 0;
	nanosleep(&req, 0);
    }
    return 0;
#endif
#elif defined(XILKERNEL) && !defined(STANDALONE)
    return sleep(seconds);
#elif defined(XILKERNEL) && defined(STANDALONE)
    DMSG( stderr, "not suppotted %s() function" EOL_CRLF, __func__);
    abort();
    return ~0;
#else
#error not impliment mpsx_sleep()
#endif
}


/**
 * @fn void mpsx_msleep(const unsigned int milisec)
 * @brief 指定ミリ秒間休止する。
 * @param milisec ミリ秒
 *	シグナルによって割り込まれることがあります。
 */
void mpsx_msleep(const unsigned int milisec)
{

#if defined(WIN32)
    Sleep(milisec);
#elif defined(_LIBMBMCS)
    mbmcs_msleep(milisec);
#elif defined(XILKERNEL) && defined(STANDALONE)
    DMSG( stderr, "not supported %s() function" EOL_CRLF, __func__);
    abort();
#elif  defined(XILKERNEL) && !defined(STANDALONE)
    sleep(milisec);
#elif defined(Linux) || defined(QNX) || defined(Darwin) || defined(__AVM2__)
    /**
     * @note POSIXではusleep()を使いたいが シグナルやalarm()を使ったときに問題が発生するためnanosleepで実装する。
     */
#if 0 /* legacy */
    usleep(milisec * 1000);
#else /* new */
    {
	struct timespec req;
	req.tv_sec = milisec / 1000;
	req.tv_nsec = (milisec % 1000) * 1000 * 1000;
	nanosleep(&req, 0);
    }
#endif
#elif defined(XILKERNEL) && !defined(STANDALONE)
    usleep(milisec * 1000);
#elif defined(XILKERNEL) && defined(STANDALONE)
    DMSG("not suppotted %s() function" EOL_CRLF, __func__);
    abort();
#else
#error not impliment mpsx_sleep()
#endif

    return;
}

/**
 * @fn int mpsx_sysconf(const enum_mpsx_sysconf_t name)
 * @brief 動作中の設定情報を取得する。
 * @param name enum_mpsx_sysconf_tで定義されたシステム情報名
 * @retval 0以上 MPSX_SYSCNF_PAGESIZEが指定された場合はメモリのページサイズ(バイト）
 * @retval 0以上 MPSX_SYSCNF_NPROCESSORS_CONFが指定された場合は設定されたプロセッサコア数
 * @retval 0以上 MPSX_SYSCNF_NPROCESSORS_ONLNが指定された場合は現在有効なプロセッサコア数
 * @retval 0以上 MPSX_SYSCNF_PHYS_PAGESが指定された場合はOSが認識している物理メモリのページ数
 * @retval 0以上 MPSX_SYSCNF_AVPHYS_PAGESが指定された場合は現在利用可能な物理メモリのページ数
 * @retval -1 失敗(errno = ENOTSUP その挙動はサポートされていない（実装できていない))
 */
int64_t mpsx_sysconf(const enum_mpsx_sysconf_t name)
{
#if defined( MPSX_SYSCONF_NOT_SUPPORT)
    DMSG( stderr, "not supported %s function" EOL_CRLF, __func__);
    abort();
#else /* else MPSX_SYSCONF_NOT_SUPPORT */
    switch (name) {
    case MPSX_SYSCNF_PAGESIZE:
#ifdef WIN32
	{
	    SYSTEM_INFO info;
	    GetSystemInfo(&info);

	    return info.dwPageSize;
	}
#elif defined(Linux) || defined(Darwin) || defined(QNX) || defined(__AVM2__)
	return (int64_t) sysconf(_SC_PAGESIZE);
#elif defined(_LIBMBMCS)
	return mbmcs_sysconf_pagesize();
#elif ( defined(XILKERNEL) && defined(MICROBLAZE) ) || defined(MICROBLAZE_MCS)
	/* ブロックメモリをメインメモリとする場合は、物理メモリーをページとして管理していない */
	/* 挙動を調べて値を正しくセットする必要がある */
	DMSG(stderr, "not impliment mpsx_sysconf(MPSX_SYSCNF_PAGESIZE) on MICROBLAZE" EOL_CRLF);
	errno = ENOTSUP;
	return -1;
#else
#error  'not impliment multios_sysconf(MPSX_SYSCNF_PAGESIZE)'
#endif
	break;
    case MPSX_SYSCNF_NPROCESSORS_CONF:
#ifdef WIN32
	{
	    SYSTEM_INFO info;
	    GetSystemInfo(&info);

	    return (int64_t) info.dwNumberOfProcessors;
	}
#elif defined(Linux) || defined(Darwin) || defined(__AVM2__)
	// for linux
	return (int64_t) sysconf(_SC_NPROCESSORS_CONF);
#elif defined(QNX)
#if _NTO_VERSION < 632
	{
	    int cores;
	    size_t len = sizeof(cores);
	    int mib[2];
	    mib[0] = CTL_HW;
	    mib[1] = HW_NCPU;
	    if (sysctl(mib, 2, &cores, &len, NULL, 0) != 0) {
		perror("sysctl");
		return -1;
	    }
	    return cores;
	}
#else
	return (int64_t) (RMSK_SIZE(_syspage_ptr->num_cpu));
#endif
#elif defined(_LIBMBMCS)
	return mbmcs_sysconf_nprocessor_conf();
#elif (defined(XILKERNEL) && defined(MICROBLAZE_MCS)) || defined(MICROBLAZE)
	/* MicroBlaze では デュアルCPU構成にしても個別のカーネルで動作するため */
	return 1;
#else
	// not impliment
#error  'not impliment multios_sysconf(MPSX_SYSCNF_NPROCESSORS_CONF)'
	return -1;
#endif
	break;
    case MPSX_SYSCNF_NPROCESSORS_ONLN:
#ifdef WIN32
	{
	    int v = 0;
	    unsigned int i;
	    SYSTEM_INFO info;
	    uint64_t dwdwActiveProcessorMask;

	    GetSystemInfo(&info);
	    dwdwActiveProcessorMask =
		(uint64_t) info.dwActiveProcessorMask;

	    for (i = 0; i < (unsigned int) (sizeof(uint64_t) * 8); i++) {
		if (dwdwActiveProcessorMask & 0x1) {
		    v++;
		}
		dwdwActiveProcessorMask >>= 1;
	    }

	    return (int64_t) v;
	}
#elif defined(Linux) || defined(Darwin) || defined(__AVM2__)
	// for POSIX standard
	return (int64_t) sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(QNX)
#if _NTO_VERSION < 632
	/* QNX6.3.2より前ではCPUを制限する方法がないため同じです。 */
	{
	    int cores;
	    size_t len = sizeof(cores);
	    int mib[2];
	    mib[0] = CTL_HW;
	    mib[1] = HW_NCPU;
	    if (sysctl(mib, 2, &cores, &len, NULL, 0) != 0) {
		perror("sysctl");
		return -1;
	    }
	    return (int64_t) cores;
	}
#else				/*  qnx 6.3.2 or more */
	{
	    int v = 0, i = 0;
	    int rsize, size_tot;
	    int *rsizep, *freep;
	    unsigned *rmaskp, *inheritp;

	    rsize = RMSK_SIZE(_syspage_ptr->num_cpu);

	    size_tot = sizeof(*rsizep);
	    size_tot += sizeof(*rmaskp) * rsize;
	    size_tot += sizeof(*inheritp) * rsize;

	    rsizep = freep = mpsx_malloca(size_tot);
	    memset(rsizep, 0x0, size_tot);

	    *rsizep = rsize;
	    rmaskp = (unsigned *) (rsizep + 1);
	    inheritp = rmaskp + rsize;

	    if (ThreadCtl(_NTO_TCTL_RUNMASK_GET_AND_SET_INHERIT, rsizep) ==
		-1) {
		perror("_NTO_TCTL_RUNMASK_GET_AND_SET_INHERIT");
		mpsx_freea(freep);
		return -1;
	    }

	    for (i = 0; i < rsize; i++) {
		if (RMSK_ISSET(i, inheritp)) {
		    v++;
		}
	    }
	    mpsx_freea(freep);

	    return v;
	}
#endif /* end of QNX _NTO_VERSION */

#elif defined(_LIBMBMCS)
	return mbmcs_sysconf_nprocessor_onln();
#elif defined(XILKERNEL) && (defined(MICROBLAZE_MCS) || defined(MICROBLAZE))
	/* MicroBlaze では デュアルCPU構成にしても個別のカーネルで動作するため */
	return 1;
#else				/* other */
#error  'not impliment mpsx_sysconf(MPSX_SYSCNF_NPROCESSORS_ONLN)'
	return -1;
#endif
	break;
    case MPSX_SYSCNF_PHYS_PAGES:
#if defined(Linux) || defined(__AVM2__) || defined(XILKERNEL)
	/* posix standard */
	return (int64_t) sysconf(_SC_PHYS_PAGES);
#elif defined(WIN32)
	{
	    MEMORYSTATUSEX memstat;
	    SYSTEM_INFO info;
	    int64_t phys_pages;

	    memstat.dwLength = (DWORD) sizeof(memstat);
	    if (0 == GlobalMemoryStatusEx(&memstat)) {
		DBMS1(stderr,
		      "mpsx_sysconf : GlobalMemoryStatusEx fail" EOL_CRLF);
		return -1;
	    }

	    GetSystemInfo(&info);

	    phys_pages = (int64_t) memstat.ullTotalPhys / info.dwPageSize;

	    return phys_pages;
	}
#elif defined(Darwin)
	{
	    struct host_basic_info binfo;
	    mach_msg_type_number_t sizeof_binfo = HOST_BASIC_INFO_COUNT;
	    int64_t pagesize, phys_pages;

	    pagesize = sysconf(_SC_PAGESIZE);
	    if (pagesize < 0) {
		DBMS1(stderr,
		      "mpsx_sysconf : sysconf(_SC_PAGESIZE) fail" EOL_CRLF);
		return -1;
	    }

	    if (host_info
		(mach_host_self(), HOST_BASIC_INFO, (host_info_t) & binfo,
		 &sizeof_binfo) != KERN_SUCCESS) {
		DBMS1(stderr, "mpsx_sysconf : host_info fail" EOL_CRLF);
		return -1;
	    }
	    phys_pages = binfo.max_mem / pagesize;
	    return phys_pages;
	}
#elif defined(QNX)
	{
	    char *str = SYSPAGE_ENTRY(strings)->data;
	    struct asinfo_entry *as = SYSPAGE_ENTRY(asinfo);
	    int64_t total = 0, total_pages, page_size;
	    uint64_t num;


	    for (num = (_syspage_ptr->asinfo.entry_size / sizeof(*as));
		 num > 0; --num) {
		if (strcmp(&str[as->name], "ram") == 0) {
		    total += as->end - as->start + 1;
		}
		++as;
	    }
	    page_size = (int64_t) sysconf(_SC_PAGESIZE);
	    total_pages = total / (int64_t) sysconf(_SC_PAGESIZE);
	    return total_pages;
	}
#elif defined(MICROBLAZE) && defined(STANDALONE)
	return mbmcs_sysconf_phys_pages();
#elif defined(OTHERS)
	/* ブロックメモリをメインメモリとする場合は、物理メモリーをページとして管理していない */
	/* 挙動を調べて値を正しくセットする必要がある */
	DMSG(stderr, "not impliment mpsx_sysconf(MPSX_SYSCNF_PHYS_PAGES) on MICROBLAZE" EOL_CRLF);
	return -1;
#else				/* other */
#error  'not impliment mpsx_sysconf(MPSX_SYSCNF_PHYS_PAGES)'
#endif
	break;
    case MPSX_SYSCNF_AVPHYS_PAGES:
#if defined(Linux)
	/* posix standard */
	return (int64_t) sysconf(_SC_AVPHYS_PAGES);
#elif defined(WIN32)
	{
	    MEMORYSTATUSEX memstat;
	    SYSTEM_INFO info;
	    int64_t free_pages;

	    memstat.dwLength = (DWORD) sizeof(memstat);
	    if (0 == GlobalMemoryStatusEx(&memstat)) {
		DBMS1(stderr,
		      "mpsx_sysconf : GlobalMemoryStatusEx fail" EOL_CRLF);
		return -1;
	    }

	    GetSystemInfo(&info);

	    free_pages = (int64_t) memstat.ullAvailPhys / info.dwPageSize;

	    return free_pages;
	}
#elif defined(Darwin)
	{
	    struct vm_statistics vm_info;
	    mach_msg_type_number_t sizeof_vm_info = HOST_VM_INFO_COUNT;
	    if (host_statistics
		(mach_host_self(), HOST_VM_INFO, (host_info_t) & vm_info,
		 &sizeof_vm_info) != KERN_SUCCESS) {
		DBMS1(stderr, "mpsx_sysconf : host_statistics fail" EOL_CRLF);
		return -1;
	    }

	    return (int64_t) vm_info.free_count;
	}
#elif defined(QNX)
	{
	    struct stat64 buf64;
	    int64_t page_size;

	    if (stat64("/proc", &buf64) == -1) {
		return (-1);
	    }

	    page_size = (int64_t) sysconf(_SC_PAGESIZE);
	    return (int64_t) buf64.st_size / page_size;
	}
#elif defined(__AVM2__)
	{
	    int64_t page_size, freemem;
	    double dfreemem;	/* numオブジェクトはdoubleで受ける */

	  inline_as3("import flash.system.System;\n" "%0 = System.freeMemory;\n": "=r"(dfreemem):
	    );
	    page_size = (int64_t) sysconf(_SC_PAGESIZE);
	    freemem = (dfreemem / (double) page_size);
	    return freemem;
	}
#elif defined(_LIBMBMCS)
	return mbmcs_sysconf_avphys_pages();
#elif defined(XILKERNEL) && ( defined(MICROBLAZE_MCS) || defined(MICROBLAZE) )
	/* ブロックメモリをメインメモリとする場合は、物理メモリーをページとして管理していない */
	/* 挙動を調べて値を正しくセットする必要がある */
	DMSG(stderr, "not impliment mpsx_sysconf(MPSX_SYSCNF_AVPHYS_PAGES) on MICROBLAZE" EOL_CRLF);
	return -1;
#else				/* other */
#error  'not impliment mpsx_sysconf(MPSX_SYSCNF_AVPHYS_PAGES)'
#endif
    default:
	return -1;
    }

    return -1;
#endif /* end of MPSX_SYSCONF_NOT_SUPPORT */
}

/**
 * @fn int mpsx_is_user_an_admin(void)
 * @brief 実行ユーザーがAdmin(root)権限を所有しているか確認します。
 * @retval 0 権限無効
 * @retval 0 以外有効
 */
int mpsx_is_user_an_admin(void)
{
#ifdef WIN32
    HANDLE hToken;
    PTOKEN_GROUPS pGroups;
    DWORD groupsLength;
    PSID administratorsSid;
    BOOL isAdministrators = FALSE;
    SID_IDENTIFIER_AUTHORITY sidIdentifier = { SECURITY_NT_AUTHORITY };
    unsigned int i;

    /* initial */

    OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);

    GetTokenInformation(hToken, TokenGroups, NULL, 0, &groupsLength);

    pGroups = (PTOKEN_GROUPS) HeapAlloc(GetProcessHeap(), 0, groupsLength);

    GetTokenInformation(hToken, TokenGroups, pGroups, groupsLength,
			&groupsLength);

    CloseHandle(hToken);

    AllocateAndInitializeSid(&sidIdentifier, 2,
			     SECURITY_BUILTIN_DOMAIN_RID,
			     DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
			     &administratorsSid);

    for (i = 0; i < pGroups->GroupCount; i++) {
	BOOL bEnabled = pGroups->Groups[i].Attributes & SE_GROUP_ENABLED;
	if (EqualSid(pGroups->Groups[i].Sid, administratorsSid)
	    && bEnabled) {
	    isAdministrators = TRUE;
	    break;
	}
    }

    FreeSid(administratorsSid);
    HeapFree(GetProcessHeap(), 0, pGroups);

    return (isAdministrators == TRUE) ? 1 : 0;
#elif defined(Linux) || defined(Darwin) || defined(QNX) || defined(__AVM2__)
    /* POSIX */
    return (geteuid() == 0) ? 1 : 0;
#elif defined(MICROBLAZE) && defined(STANDALONE)
    return 1;
#else
#error  'can not defined mpsx_is_user_an_admin()'
#endif
}

/**
 * @fn int mpsx_fork(void)
 * @brief 呼び出し元プロセスを複製して新しいプロセスを生成する。 
 *     child で参照される新しいプロセスは、以下の点を除き、 parent で参照される呼び出し元プロセスの完全な複製である
 *     ※ MS-Windows系では実装されていません
 * @retval 0以上 成功。親プロセスには子プロセスの PID が返され、 子プロセスには 0 が返される。 
 * @retval -1 失敗。親プロセスに -1 が返され、子プロセスは生成されず、 errno が適切に設定される。   
 *	EAGAIN　親プロセスのページ・テーブルのコピーと 子プロセスのタスク構造に生成に必要なメモリを fork() が割り当てることができなかった。
 *	        呼び出し元の RLIMIT_NPROC 資源の制限 (resource limit) に達したために、新しいプロセスを生成できなかった。 
 *      ENOMEM  メモリが足りないために、 fork() は必要なカーネル構造体を割り当てることができなかった。
 *      ENOSYS  fork() はこのプラットフォームではサポートされていない
 **/
int mpsx_fork(void)
{
#if (defined(MICROBLAZE) && defined(STANDALONE)) || defined(MPSX_FORK_NOT_SUPPORT)
	DMSG( stderr, "not supported %s function \r\n", __func__);
	errno = ENOSYS;
	return -1;
#else /* MPSX_FORK_NOT_SUPPORT */
#if defined(Linux) || defined(Darwin) || defined(QNX) || defined(__AVM2__)
    return fork();
#elif defined(WIN32)
    errno = ENOSYS;
    return -1;
#else /* others OS */
#error  'can not defined mpsx_fork()'
#endif
#endif /* end of MPSX_FORK_NOT_SUPPORT */
}

/**
 * @fn int mpsx_get_username(char *name_p, const size_t maxlen)
 * @brief 現在ログインして使用中のユーザー名を戻します ** 動作未検証 **
 * 	マルチスレッドで呼ぶには外で排他処理してね
 * @param name_p ユーザー名取得用のバッファポインタ
 * @param maxlen バッファの長さ
 * @retval 0 成功
 * @retval EFAULT nameが不正なアドレス
 * @retval EINVAL len値が不正(許容サイズを超えてる） *
 **/
int mpsx_get_username( char *name, const size_t maxlen)
{
#if defined(Linux) || defined(Darwin) || defined(QNX) || defined(__AVM2__)
    struct passwd *pw = NULL;

    /* ユーザ名取得 */
    pw = getpwuid(getuid());
    if( NULL == pw ) {
	return errno;
    }
    if(!(strlen(pw->pw_name) < maxlen)) {
	return EINVAL;
    }

    strcpy( name, pw->pw_name);

    return 0;

#elif defined(WIN32)
    DWORD dwResult;
    DWORD dwSize = (DWORD)maxlen;
    BOOL bRet;

    bRet = GetUserName( name , &dwSize);
    if (0 != bRet) {
	/* 成功 */
	return 0;
    }

    dwResult = GetLastError();
    if(dwResult == ERROR_BUFFER_OVERFLOW) {
	return EINVAL;
    }

    return EFAULT;

#else /* MPSX_GET_USERNAME_NOT_SUPPORT */

    return ENOSYS;
#if 0
#error  'can not defined mpsx_get_username()'
    return ENOSUP;
#endif
#endif
}

/**
 * @fn int mpsx_get_hostname( char * name, const size_t maxlen)
 * @brief ホストPC名を取得する ** 動作未検証 **
 * @param name ホストPC名取得バッファ
 * @param maxlen nameバッファの最大文字列数
 * @retval 0 成功
 * @retval EFAULT nameが不正なアドレス
 * @retval EINVAL len値が不正(許容サイズを超えてる）
 **/
int mpsx_get_hostname( char *name, const size_t maxlen)
{
#if defined(Linux) || defined(Darwin) || defined(QNX) || defined(__AVM2__)
    return gethostname(name , maxlen);
#elif defined(WIN32)
    BOOL bRet;
    DWORD dwSize = (DWORD)maxlen;
    DWORD dwResult;
    bRet = GetComputerName( name, &dwSize);
    if(bRet !=0 ) {
	/* 成功 */
	return 0;
    }

    dwResult = GetLastError();
    if(dwResult == ERROR_BUFFER_OVERFLOW) {
	return EINVAL;
    }
    return EFAULT;
#else /* MPSX_GET_HOSTNAME_NOT_SUPPORT */

    return ENOSYS;
#if 0
#error  'can not defined mpsx_get_hostname()'
    return ENOSUP;
#endif
#endif

}

