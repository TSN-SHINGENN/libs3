/**
 *	Copyright 2018 TSNｰSHINGENN .All Rights Reserved.
 *
 *	Basic Author: Seiichi Takeda  '2018-April-06 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @file multios_socket_server_process_fork.c
 * @brief サーバープロセスでforkを実現するためのオブジェクトクラスライブラリ
 * 	WIN32系でfork()が存在しないため、子プロセス生成プロセスを代用します。
 * 	POSIX系では通常のFORKと同じ扱いで処理されます。
 *      
 */

/* POSIX & C Runtime */
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <stdlib.h>
#include <malloc.h>

#ifdef WIN32
#include <windows.h>
#endif

/* libmultios */
#include "multios_sys_types.h"
#include "multios_getopt.h"
#include "multios_malloc.h"
#include "multios_stl_vector.h"
#include "multios_stl_string.h"

/* this */
#include "multios_socket_server_process_fork.h"

#ifdef DEBUG
static int debuglevel = 1;
#else
static const int debuglevel = 3;
#endif
#include "dbms.h"

typedef struct _multios_socket_server_process_fork_ext {
    multios_stl_vector_t vector_handle_pool;
    multios_getopt_t getopt_long;
    multios_stl_string_t args_string;
    union {
	uint32_t flags;
	struct { 
	    uint32_t getopt_long:1;
	    uint32_t vector_handle_pool:1;
	    uint32_t args_string:1;
	} f;
    } init;
} multios_socket_server_process_fork_ext_t;

#define get_socket_server_process_fork_ext(s) MULTIOS_STATIC_CAST(multios_socket_server_process_fork_ext_t*,(s)->ext)


/**
 * @fn int multios_socket_server_process_fork_init(multios_socket_server_process_fork_t *const self_p, const int listen_port_no)
 * @brief multios_socket_server_process_fork_tインスタンスを初期化します。
 * @param self_p multios_socket_server_process_fork_t構造体インスタンスポインタ
 * @param listen_port_no 子プロセスが主に使用するポート番号
 * @retval 0 成功
 * @retval EINVAL 不正な引数
 * @retval EAGAIN 必要なリソースを確保できなかった
 **/
int multios_socket_server_process_fork_init(multios_socket_server_process_fork_t *const self_p, const int listen_port_no)
{
    int status, result;
    multios_socket_server_process_fork_ext_t *ext=NULL;

    if( NULL == self_p ) {
	return EINVAL;
    } else if((-1 > listen_port_no) || (listen_port_no > 65535)) {
	return EINVAL;
    }

    /* initilalize */
    memset( self_p, 0x0, sizeof(multios_socket_server_process_fork_t));

    ext = (multios_socket_server_process_fork_ext_t*)multios_malloc( sizeof(multios_socket_server_process_fork_ext_t));
    if( NULL == ext ) {
	DBMS1( stdout, __func__ " : malloc(multios_socket_server_process_fork_ext_t) fail\n");
	status = EAGAIN;
	goto out;
    }
    self_p->ext = MULTIOS_STATIC_CAST( void*, ext);

    result = multios_stl_vector_init( &ext->vector_handle_pool, sizeof(int));
    if(result) {
	DBMS1( stdout, __func__ " : multios_stl_vector_init(vector_handle_pool) fail\n");
	status = result;
	goto out;
    } else {
	ext->init.f.vector_handle_pool = 1;
    }

    result = multios_stl_string_init(&ext->args_string);
    if(result) {
	DBMS1( stderr, __func__ " : multios_stl_string_init(args_string) fail, strerr=%s\n",
			strerror(result));
	status = result;
	goto out;
    } else {
	ext->init.f.args_string = 1;
    }

    result = multios_socket_server_process_fork_set_listen_port_no( self_p,listen_port_no);
    if(result) {
	DBMS1( stderr, __func__ " : multios_socket_server_process_fork_set_listen_port_no fail, strerr=%s\n",
			strerror(result));
	status = result;
	goto out;
    }

    ext->getopt_long = multios_getopt_initializer();
    ext->init.f.getopt_long = 1;
    status = 0;

out:
    if(status) {
	multios_socket_server_process_fork_destroy(self_p);
    }

    return status;
}

/**
 * @fn int multios_socket_server_process_fork_set_listen_port_no(multios_socket_server_process_fork_t *const self_p)
 * @brief ネットワークのリッスンポート番号を設定します。
 *	ポート番号は 0-65535 まで, また-1で無効設定
 * @param self_p multios_socket_server_process_fork_t構造体インスタンスポインタ
 * @param listen_port_no ネットワークのリッスンポート番号。-1指定で無効に変更
 * @retval 0 成功
 * @retval EINVAL 引数不正
 **/
int multios_socket_server_process_fork_set_listen_port_no(multios_socket_server_process_fork_t *const self_p,const int listen_port_no)
{
    multios_socket_server_process_fork_ext_t *const ext = NULL;

    if( NULL == self_p ) {
	return EINVAL;
    }
    DBMS3( stderr, __func__ " : execute self_p=%p,listen_port_no=%d|n", self_p, listen_port_no );

    if((listen_port_no < -1) || (listen_port_no > 65535)) {

	return EINVAL;
    }

    if( listen_port_no < 0 ) {
	self_p->listen_port_no = ~0;
        self_p->stat.f.listen_port_no = 0;
    } else {
	self_p->listen_port_no = listen_port_no;
        self_p->stat.f.listen_port_no = 1;
    }
    return 0;
}

/**
 * @fn int multios_socket_server_process_fork_destroy(multios_socket_server_process_fork_t *const self_p)
 * @brief multios_socket_server_process_fork_tインスタンスを破棄します。
 * @param self_p multios_socket_server_process_fork_t構造体インスタンスポインタ
 * @retval 0 成功
 * @retval EINVAL 不正な引数
 * @retval EAGAIN 必要なリソースを確保できなかった
 **/
int multios_socket_server_process_fork_destroy(multios_socket_server_process_fork_t *const self_p)
{
    multios_socket_server_process_fork_ext_t *ext = NULL;
    int result;

    if( NULL == self_p ) {
	return EINVAL;
    }

    ext =  get_socket_server_process_fork_ext(self_p);
    ext->init.f.getopt_long = 0;

    if(ext->init.f.vector_handle_pool) {;
	result = multios_stl_vector_destroy( &ext->vector_handle_pool);
	if(result) {
	    DBMS1( stdout, __func__ " : multios_stl_vector_destroy(vector_handle_pool) fail, strerror=%s\n", strerror(result));
	    return result;
	}
	ext->init.f.vector_handle_pool = 0;
    } 

    if(ext->init.f.args_string) {
	multios_stl_string_destroy(&ext->args_string);
	ext->init.f.args_string = 0;
    }

    if( ext->init.flags ) {
	DBMS1( stderr, __func__ " : ext->init.flags = 0x%08x\n", ext->init.flags );
	return -1;
    } else {
	multios_free(self_p);
    }
    
    return 0;
}

#define ARGSTR_CHILDSERVER "ChildServer"
#define ARGSTR_HANDLEINHERITANCE "HandleInheritance"
#define ARGSTR_LISTERNPORT "ListenPort"
#define ARGSTR_CHILDSERVER_FINI "ChildServerFINI"

multios_getopt_long_option_t socket_server_option[] = {
    { ARGSTR_CHILDSERVER      , MULTIOS_GETOPT_NO_ARGUMENT, NULL, 'c'       },
    { ARGSTR_HANDLEINHERITANCE, MULTIOS_GETOPT_REQUIRED_ARGUMENT, NULL, 'h' },
    { ARGSTR_LISTERNPORT      , MULTIOS_GETOPT_REQUIRED_ARGUMENT, NULL, 'p' },
    { ARGSTR_CHILDSERVER_FINI , MULTIOS_GETOPT_NO_ARGUMENT, NULL, 'f'       },
    { ""                      , MULTIOS_GETOPT_eot              , NULL, '\0'},
};

static const char *const socket_server_short_option = "ch:p:f";

/**
 * @fn int multios_socket_server_process_fork_getopt_long(multios_socket_server_process_fork_t *const self_p, int *const retnumprocs_p, const int argc, const char *const argv[], int *longindex_p)
 * @brief WIN32の場合、fork()が存在しないので、ハンドルを引数で継承するようにする
 * @param self_p multios_socket_server_process_fork_t構造体インスタンスポインタ
 * @param retnumprocs_p 処理後の配列数を戻すための変数ポインタ(NULLで戻さない)
 * @param argc getoptに渡すための引数数
 * @param argv getoptに渡すための引数ポインタ
 * @retval 0 成功
 *
*/
int multios_socket_server_process_fork_getopt_long(multios_socket_server_process_fork_t *const self_p, int *const retnumprocs_p, const int argc, const char *const argv[], int *const longindex_p)
{
    multios_socket_server_process_fork_ext_t * const ext = get_socket_server_process_fork_ext(self_p);
    int result, status;
    int opt;
    int longindex;

    if( NULL == self_p ) {
	return EINVAL;
    }

   /* 引数を解析　*/
    while(( opt = multios_getopt_long_only( &ext->getopt_long, argc, argv, socket_server_short_option, socket_server_option, &longindex)) != -1 ) {
	switch(opt) {
	    case 'c': {
		/* ARGSTR_CHILDSERVER */
		DBMS3( stderr, __func__ " : ARGSTR_CHILDSERVER Option\n");
		self_p->stat.f.is_child = 1;
	    }
	    break;

	    case 'h': {
		unsigned int numofhandle = 0;
		unsigned int n;

		/* ARGSTR_HANDLEINHERITANCE */
		DBMS3( stderr, __func__ " : ARGSTR_HANDLEINHERITANCE Option\n");
		result = multios_stl_vector_clear( &ext->vector_handle_pool);
		if(result) {
		    DBMS1( stderr, __func__ " : multios_stl_vector_clear(vector_handle_poolinvalid num fail, strerror=%s", strerror(result));
		    status = result;
		    goto out;
		}

		if( sscanf( ext->getopt_long.optarg, "%i", &numofhandle) != 1 ) {
		    DBMS1( stderr, "__func__ : invalid num %s\n",ARGSTR_HANDLEINHERITANCE);
		    status = EINVAL;
		    goto out;
		}

		for( n=0; n<numofhandle; ++n) {
		    unsigned int a;
		    if( sscanf( argv[ext->getopt_long.optind],"%i", &a) != 1) {
			DBMS1( stderr, __func__ " : %s invalid num %s\n", ARGSTR_HANDLEINHERITANCE,argv[ext->getopt_long.optind]);
			status = EINVAL;
			goto out;
		    }
		    ++(ext->getopt_long.optind);
		    DBMS3( stderr, __func__ " : %s handle[%i]=%d\n", ARGSTR_HANDLEINHERITANCE, n, a);
		    result = multios_stl_vector_push_back( &ext->vector_handle_pool, &a, sizeof(unsigned int));
		    if(result) {
			DBMS1( stderr, "__func__ : multios_stl_vector_push_back(vector_handle_pool) fail, strerror=%s\n", strerror(result));
			status = result;
			goto out;
		    }
		}    
	    }
	    break;

	    case 'p': {
		/* ARGSTR_LISTERNPORT */
		DBMS3( stderr, __func__ " : ARGSTR_LISTERNPORT Option\n");

		if( sscanf( ext->getopt_long.optarg, "%d", &self_p->listen_port_no) != 1 ) {
		    DBMS1( stderr, __func__ " : invalid num %s = %s\n",ARGSTR_HANDLEINHERITANCE,
			ext->getopt_long.optarg);
		    status = EINVAL;
		    goto out;
		}
		DBMS3( stderr, __func__ " : listen_port_no = %d\n", self_p->listen_port_no);
		self_p->stat.f.listen_port_no = 1;
	    }
	    break;

	    case 'f': {
		/* ARGSTR_CHILDSERVER_FINI */
		DBMS3( stderr, __func__ " : ARGSTR_CHILDSERVER_FINI Option\n");
		status = 0;
		goto out;
	    }
	    break;

	    default: {
		/* Invalid Option */
		DBMS3( stderr, __func__ " : ARGSTR_CHILDSERVER_FINI Option\n");
		status = EINVAL;
		goto out;
	    }
	}
    }
	status = 0;

out:

    if(!status) {
	if( NULL != longindex_p) {
	    *longindex_p = longindex;
	}
	if( NULL != retnumprocs_p ) {
	    *retnumprocs_p = ext->getopt_long.optind;
	}
    }

    return status;
}

/** フォーク時に継承するハンドルを設定します。
 * @fn int multios_socket_server_process_fork_set_Handle_inheritance(multios_socket_server_process_fork_t *const self_p, const int num_hanldes, const int *const handle_arrays)
 * @param self_p multios_socket_server_process_fork_t構造体インスタンスポインタ
 * @param num_handles 子スレッドに継承するハンドル数
 * @param handle_arrays 子スレッドに継承するハンドル配列
 * @retval 0 成功
 * @retval EINVAL 不正な引数がある
 * @retval ENONMEM ハンドル
 **/
int multios_socket_server_process_fork_set_Handle_inheritance(multios_socket_server_process_fork_t *const self_p, const size_t numof_hanldes, const int *const handle_arrays)
{
    int result;
    size_t n;
    multios_socket_server_process_fork_ext_t *const ext =  get_socket_server_process_fork_ext(self_p);

    if( NULL == self_p ) {
	return EINVAL;
    }

    result = multios_stl_vector_clear( &ext->vector_handle_pool);
    if(result) {
	DBMS1( stderr, __func__ " : multios_stl_vector_clear(vector_handle_pool) fail, strerror=%s", strerror(result));
	return result;
    }

    for( n=0; n<numof_hanldes; ++n ) {
  	result = multios_stl_vector_push_back( &ext->vector_handle_pool, &handle_arrays[n], sizeof(unsigned int)); 
	if(result) {
	    DBMS1( stderr, __func__ " : multios_stl_vector_push_back(vector_handle_pool) fail, strerror=%s", strerror(result));
	    return result;
	}
    }

    return 0;
}


/**
 * @fn int multios_socket_server_process_fork(multios_socket_server_process_fork_t *const self_p, char *const execute child_process_binary_file)
 * @brief listen状態に入ります。
 * @retval 0 子プロセスです。
 * @retval  1以上 親プロセスです、
 * @retval エラーです
 */
int multios_socket_server_process_fork(multios_socket_server_process_fork_t *const self_p, char *file_path, char *const child_process_binary_filename)
{
    multios_socket_server_process_fork_ext_t *const ext =  get_socket_server_process_fork_ext(self_p);

#ifdef WIN32    
    int result, status;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    /* check */
    if( NULL == self_p ) {
	return EINVAL;
    }

    // initialize
    result = multios_stl_string_clear(&ext->args_string);
    if(result) {
	DBMS1( stderr, __func__ " : multios_stl_string_clear fail, strerr=%s\n",
			strerror(result));
	status = result;
	goto out;
    }

    // process parameter
    ZeroMemory( &si, sizeof(STARTUPINFO) );
    memset( &si, 0x0, sizeof( STARTUPINFO ) );

    si.cb = sizeof( si );
    si.hStdOutput = stdout;

    ZeroMemory( &pi, sizeof(PROCESS_INFORMATION) ); 

    result = multios_stl_string_append(&ext->args_string, " -" ARGSTR_CHILDSERVER);
    if(result) {
	DBMS1( stderr, __func__ " : multios_stl_string_append(ARGSTR_CHILDSERVER) fail, strerror=%s\n",
			strerror(result));
	status = result;
	goto out;
    }

    if(self_p->stat.f.listen_port_no) {
	char *sentence = multios_stl_new_sprintf_data(" -%s %d",ARGSTR_LISTERNPORT,self_p->listen_port_no);
 	if( NULL == sentence ) {
	    DBMS1( stderr, __func__ " : multios_stl_new_sprintf_data(ARGSTR_LISTERNPORT) fail\n");
	    status = -1;
	    goto out;
	}
	result = multios_stl_string_append(&ext->args_string, sentence);
        if(result) {
	    DBMS1( stderr, __func__ " : multios_stl_string_append(ARGSTR_CHILDSERVER) fail, strerror=%s\n",
			strerror(result));
	    multios_stl_delete_sprintf_data(sentence);
	    status = result;
	    goto out;
	}
    }

    if(!multios_stl_vector_is_empty( &ext->vector_handle_pool)) {
	const size_t num_pools = multios_stl_vector_get_pool_cnt( &ext->vector_handle_pool);
	size_t n;    
	char *sentence;

	sentence = multios_stl_new_sprintf_data(" -%s %d", ARGSTR_HANDLEINHERITANCE,(int)num_pools);
 	if( NULL == sentence ) {
	    DBMS1( stderr, __func__ " : multios_stl_new_sprintf_data(ARGSTR_HANDLEINHERITANCE) fail\n");
	    status = -1;
	    goto out;
	}
	result = multios_stl_string_append(&ext->args_string, sentence);
        if(result) {
	    DBMS1( stderr, __func__ " : multios_stl_string_append(ARGSTR_HANDLEINHERITANCE) fail, strerror=%s\n",
			strerror(result));
	    multios_stl_delete_sprintf_data(sentence);
	    status = result;
	    goto out;
	}
	multios_stl_delete_sprintf_data(sentence);

	for( n=0; n<multios_stl_vector_get_pool_cnt( &ext->vector_handle_pool); ++n) {
	    int handle = *(int*)multios_stl_vector_ptr_at( &ext->vector_handle_pool, n);
	    sentence = multios_stl_new_sprintf_data(" %d", handle);

	    result = multios_stl_string_append(&ext->args_string, sentence);
	    if(result) {
		DBMS1( stderr, __func__ " : multios_stl_string_append(%d:handle[%u] fail, strerror=%s\n",
			n, handle, strerror(result));
		multios_stl_delete_sprintf_data(sentence);
		status = result;
		goto out;
	    }
	    multios_stl_delete_sprintf_data(sentence);
	}

	result = multios_stl_string_append(&ext->args_string, " -" ARGSTR_CHILDSERVER_FINI);
	if(result) {
	    DBMS1( stderr, __func__ " : multios_stl_string_append(handle[%u] fail, strerror=%s\n",
			n, strerror(result));
	    multios_stl_delete_sprintf_data(sentence);
	    status = result;
	    goto out;
	}

	result = multios_stl_string_append(&ext->args_string, "\n");
	if(result) {
	    DBMS1( stderr, __func__ " : multios_stl_string_append(handle[%u] fail, strerror=%s\n",
			n, strerror(result));
	    multios_stl_delete_sprintf_data(sentence);
	    status = result;
	    goto out;
	}
    }

    DMSG( stderr, "args string=%s\n", multios_stl_string_ptr_at( &ext->args_string, 0));

    if(0) { /* to 0 for debug */
	BOOL bRet;
	char *pathfilename = multios_stl_new_sprintf_data("%s\\%s", file_path, child_process_binary_filename);
	char *file_args = multios_stl_new_sprintf_data("%s %s", child_process_binary_filename, multios_stl_string_ptr_at( &ext->args_string, 0));

	bRet = CreateProcess(
	    pathfilename,
	    file_args,
	    NULL,
	    NULL,
	    TRUE,  // 引数の継承を指定する
	    NORMAL_PRIORITY_CLASS,
	    NULL,
	    NULL,
	    &si, &pi);

 	if ( bRet ) {
	    DMSG( stderr, "PARENT : Succeed to create child process. PID=%d\n", pi.dwProcessId );
	    CloseHandle( pi.hProcess );
	    CloseHandle( pi.hThread );
	} else {
	    printf( "PARENT : Failed to create child process.\n" );
	}

    }

    status = 0;

out:

#endif

    return status;
}

/**
 * @fn int multios_socket_server_process_is_child(multios_socket_server_process_fork_t *const self_p)
 * @brief 自分が子プロセスなのかどうかを確認する
 * @brief 0 子プロセスではない(又は処理エラー
 * @brief 1以上 子プロセスだ
 */
int multios_socket_server_process_is_child(multios_socket_server_process_fork_t *const self_p)
{
    if(NULL == self_p) {
	return -1;
    }
    if(NULL == get_socket_server_process_fork_ext(self_p)) {
	return -1;
    }

    return (self_p->stat.f.is_child) ? 1 : 0;
}

const char *_multios_socket_server_get_args_string(multios_socket_server_process_fork_t *const self_p)
{
    multios_socket_server_process_fork_ext_t *const ext =  get_socket_server_process_fork_ext(self_p);

    /* check */
    if( NULL == self_p ) {
	return NULL;
    }

    return (const char *)multios_stl_string_ptr_at( &ext->args_string, 0);
}

void _multios_socket_server_process_fork_dump_params( multios_socket_server_process_fork_t *const self_p)
{
    multios_socket_server_process_fork_ext_t *const ext =  get_socket_server_process_fork_ext(self_p);

    /* check */
    if( NULL == self_p ) {
	DMSG( stderr, __func__ " : self_p is NULL\n");
	return;    
    }

    DMSG( stderr, __func__ " : self_p->stat.f.listen_port_no=%u listen_port_no=%d\n",
	self_p->stat.f.listen_port_no, self_p->listen_port_no);
    DMSG( stderr, __func__ " : self_p->stat.f.is_child=%u\n", self_p->stat.f.listen_port_no);

    return;
}

