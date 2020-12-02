/**
 *      Copyright 2012 TSN-SHINGENN All Rights Reserved.
 *
 *      Basic Author: Seiichi Takeda  '2014-July-02 Active
 *              Last Alteration $Author: takeda $
 */

/**
 * @file mtxx_omp_parallelfor.c
 * @brief duration指定から処理をスレッドで分割して実行します。
 *	OpenMPをエミュレートするライブラリの一部です。
 *	parallel for構文と同等(以上)の機能をライブラリ化しています。
 *	MACOSX10.5/10.6系の標準のgccではopenMPの動作に制限があるため、代替案として作成されました。
 */

#ifdef WIN32
#if _MSC_VER >= 1400 /* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* libs3 */
#include <libmpxx/mpxx_sys_types.h>
#include <libmpxx/mpxx_stdlib.h>
#include <libmpxx/mpxx_unistd.h>
#include <libmpxx/mpxx_stl_vector.h>

#ifdef DEBUG
static int debuglevel = 1;
#else
static const int debuglevel = 0;
#endif
#include <libmpxx/dbms.h>

/* mtxx */
#include <libmtxx/mtxx_pthread.h>
#include <libmtxx/mtxx_pthread_mutex.h>
#include <libmtxx/mtxx_pthread_once.h>
#include <libmtxx/mtxx_pthread_barrier.h>
#include <libmtxx/mtxx_semaphore.h>
#include <libmtxx/mtxx_omp_parallelfor.h>

#if defined(_LIBS3_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

typedef enum _enum_dynamic_state_t {
    DSTATE_EXEC,
    DSTATE_EOL,
    DSTATE_eos,
} enum_dynamic_state_t;

typedef struct _parallelfor_worker {
    size_t no;
    s3::mtxx_pthread_t pth;
    void *ext_p;
    size_t start_pos;
    size_t duration;

    union {
	unsigned int flags;
	struct {
	    unsigned int pth:1;
	} f;
    } init;
} parallelfor_worker_t;

typedef struct _omp_parallelfor_ext {
    size_t num_threads;
    size_t num_baris;
    s3::mtxx_sem_t st_sem;
    s3::mtxx_pthread_barrier_t end_bari;
    void *func_args;
    s3::mtxx_omp_parallelfor_callback_t job_func;
    s3::mtxx_pthread_mutex_t mutex;

    s3::mtxx_omp_parallelfor_attr_t attr;

    mpxx_stl_vector_t wth;

    union {
	unsigned int flags;
	struct {
	    unsigned int schedule_is_dynamic:1;
	    unsigned int finish:1;
	} f;
    } stat;

    union {
	unsigned int flags;
	struct {
	    unsigned int wth:1;
	    unsigned int st_sem:1;
	    unsigned int end_bari:1;
	    unsigned int mutex:1;
	} f;
    } init;
} omp_parallelfor_ext_t;

typedef struct _omp_parallelfor_stat {
    s3::mtxx_pthread_mutex_t intelocked_mutex;

    union {
	unsigned int flags;
	struct {
	    unsigned int intelocked_mutex:1;
	} f;
    } init;
} omp_parallelfor_stat_t;

static omp_parallelfor_stat_t omp_parallelfor_stat;

#define get_omp_parallelfor_stat() &omp_parallelfor_stat;
#define get_omp_parallelfor_ext(e) MPXX_STATIC_CAST(omp_parallelfor_ext_t*, e)

/**
 * @fn static void omp_parallelfor_once_init(void)
 * brief parallelforをアトミックに使用するための管理機能を初期化
 */
static void omp_parallelfor_once_init(void)
{
    int result;
    omp_parallelfor_stat_t *const __restrict ss =
	get_omp_parallelfor_stat();

    memset(&omp_parallelfor_stat, 0x0, sizeof(omp_parallelfor_stat_t));

    result = s3::mtxx_pthread_mutex_init(&ss->intelocked_mutex, NULL);
    if (result) {
	DBMS1(stderr,
	      "omp_parallelfor_once : s3::mtxx_pthread_mutex_init fail" EOL_CRLF);
    } else {
	ss->init.f.intelocked_mutex = 1;
    }

    return;
}

/**
 * @fn static void *th_parallelfor_static_worker(void *d)
 * @brief parallelfor_static_workerスレッド本体
 * @param d 引数ポインタ
 * @return NULL固定
 */
static void *th_parallelfor_static_worker(void *d)
{
    int result;
    parallelfor_worker_t *const p = MPXX_STATIC_CAST(parallelfor_worker_t *, d);
    omp_parallelfor_ext_t *const ext =
	get_omp_parallelfor_ext(p->ext_p);

    while (1) {
	result = s3::mtxx_sem_wait(&ext->st_sem);
	if (result) {
	    IFDBG1THEN {
		const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
		char xtoa_buf[MPXX_XTOA_DEC64_BUFSZ];
		
		mpxx_u64toadec( (uint64_t)p->no, xtoa_buf, xtoa_bufsz);
    
		DBMS1(stderr,
		  "th_parallelfor_static_worker[%s] : s3::mtxx_sem_wait(st_sem) fail" EOL_CRLF, xtoa_buf);
	    }
	    goto out;
	}

	if (ext->stat.f.finish) {
	    goto out;
	}

	if (p->duration != 0 || ext->attr.f.is_called_even0duration) {
	    (ext->job_func) ((int)p->no, ext->func_args, p->start_pos,
			     p->duration);
	} else {
	    IFDBG3THEN {
		const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
		char xtoa_buf[MPXX_XTOA_DEC64_BUFSZ];

		mpxx_u64toadec( (uint64_t)p->duration, xtoa_buf, xtoa_bufsz);
		DBMS3(stderr,
		  "th_parallelfor_static_worker : p->duration=%s, ext->attr.f.is_called_even0duration=%u" EOL_CRLF,
		    xtoa_buf, ext->attr.f.is_called_even0duration);
	    }
	}

	result = s3::mtxx_pthread_barrier_wait(&ext->end_bari);
	if (result) {
	    IFDBG1THEN {
		const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
		char xtoa_buf[MPXX_XTOA_DEC64_BUFSZ];
    
		mpxx_u64toadec( (uint64_t)p->no, xtoa_buf, xtoa_bufsz);
		DBMS1(stderr,
		  "th_parallelfor_static_worker[%s] : s3::mtxx_pthread_barrier_wait(end_bari) fail" EOL_CRLF, xtoa_buf);
	    }
	    goto out;
	}
    }

  out:

    return NULL;
}

static enum_dynamic_state_t dynamic_worker_operater( omp_parallelfor_ext_t *e, size_t *start_pos_p, size_t *duration)
{
    (void)e;
    (void)start_pos_p;
    (void)duration;

    return DSTATE_eos;
}

/**
 * @fn static void *th_parallelfor_dynamic_worker(void *d)
 * @brief parallelfor_static_dynamic_workerスレッド本体
 * @param d 引数ポインタ
 * @return NULL固定
 */
static void *th_parallelfor_dynamic_worker(void *d)
{
    int result;
    parallelfor_worker_t *const p = MPXX_STATIC_CAST(parallelfor_worker_t *, d);
    omp_parallelfor_ext_t *const e =
	get_omp_parallelfor_ext(p->ext_p);

    while (1) {
	result = s3::mtxx_sem_wait(&e->st_sem);
	if (result) {
	    IFDBG1THEN {
		const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
		char xtoa_buf[MPXX_XTOA_DEC64_BUFSZ];

		mpxx_u64toadec( (uint64_t)p->no, xtoa_buf, xtoa_bufsz);
		DBMS1(stderr,
		  "th_parallelfor_dynamic_worker[%s] : s3::mtxx_sem_wait(st_sem) fail" EOL_CRLF, xtoa_buf);
	    }
	    goto out;
	}

	if (e->stat.f.finish) {
	    goto out;
	}

	while(2) {
	    size_t start_pos;
	    size_t duration;
	    enum_dynamic_state_t state;
	    state = dynamic_worker_operater( e, &start_pos, &duration);
	    if(result == DSTATE_EOL ) {
		break;
	    }

	    if ( duration != 0 || e->attr.f.is_called_even0duration) {
		(e->job_func) ((int)p->no, e->func_args, start_pos, duration);
	    } else {
		IFDBG3THEN {
		    const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
		    char xtoa_buf_a[MPXX_XTOA_DEC64_BUFSZ];
		    char xtoa_buf_b[MPXX_XTOA_DEC64_BUFSZ];

		    mpxx_u64toadec( (uint64_t)duration, xtoa_buf_a, xtoa_bufsz),
		    mpxx_u64toadec( (uint64_t)e->attr.f.is_called_even0duration, xtoa_buf_b, xtoa_bufsz);
		    DBMS3(stderr,
			"th_parallelfor_dynamic_worker : p->duration=%s, ext->attr.f.is_called_even0duration=%s" EOL_CRLF,
			xtoa_buf_a, xtoa_buf_b);
		} 
	    }
	}

	result = s3::mtxx_pthread_barrier_wait(&e->end_bari);
	if (result) {
	    IFDBG1THEN {
		const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
		char xtoa_buf[MPXX_XTOA_DEC64_BUFSZ];

		mpxx_u64toadec( (uint64_t)p->no, xtoa_buf, xtoa_bufsz);
		DBMS1(stderr,
		  "th_parallelfor_dynamic_worker[%s] : s3::mtxx_pthread_barrier_wait(end_bari) fail" EOL_CRLF, xtoa_buf);
	    }
	    goto out;
	}
    }

  out:

    return NULL;
}


/**
 * @fn int s3::mtxx_omp_parallelfor_init(s3::mtxx_omp_parallelfor_t *self_p, s3::mtxx_omp_parallelfor_callback_t func, size_t num_threads, s3::mtxx_omp_parallelfor_attr_t *attr_p)
 * @brief s3::mtxx_omp_parallelforオブジェクトを初期化します
 * @param self_p s3::mtxx_omp_parallelfor_tインスタンスポインタ
 * @param func s3::mtxx_omp_parallelfor_func_tスレッドコール関数
 * @param num_threads 起動スレッド数(0以下で有効なCPU数に合わせます)
 * @param attr_p 属性を指定の為のフラグポインタ NULLでデフォルト動作になります。
 *	is_uninterlocked : このインスタンスが他のインスタンスと排他的に処理されないようにします
 *	is_externalwaitforjob :並列処理実行終了を s3::mtxx_omp_parallelfor_waitforjob()で待つようにします 
 * @retval EINVAL パラメータ不正
 * @retval EAGAIN リソース不足
 * @retval -1 失敗
 * @retval 0 成功
 */
int s3::mtxx_omp_parallelfor_init(s3::mtxx_omp_parallelfor_t * self_p,
				 s3::mtxx_omp_parallelfor_callback_t func,
				 size_t num_threads,
				 s3::mtxx_omp_parallelfor_attr_t * attr_p)
{
    int status, result;
    size_t n;
    static s3::mtxx_pthread_once_exec_t omp_parallelfor_once = { 0 };
    omp_parallelfor_ext_t *ext;

    if (NULL == func) {
	return EINVAL;
    }

    result = s3::mtxx_pthread_once_exec_init();
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_parallelfor_init : s3::mtxx_pthread_once_exec_init fail" EOL_CRLF);
	return -1;
    }

    result = s3::mtxx_pthread_once_exec_get_instance(&omp_parallelfor_once);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_parallelfor_init : s3::mtxx_pthread_once_exec_get_instance fail, strerror=%s" EOL_CRLF,
	      strerror(result));
	return -1;
    }

    memset(self_p, 0x0, sizeof(s3::mtxx_omp_parallelfor_t));

    if (num_threads <= 0) {
	num_threads = s3::mtxx_omp_get_num_procs();
	if (num_threads <= 0) {
	    DBMS1(stderr,
		  "s3::mtxx_omp_parallelfor_init : s3::mtxx_omp_get_num_procs fail" EOL_CRLF);
	    return -1;
	}
    }

    ext = (omp_parallelfor_ext_t *) malloc(sizeof(omp_parallelfor_ext_t));
    if (NULL == ext) {
	DBMS1(stderr, "s3::mtxx_omp_parallelfor_init : malloc(ext) fail" EOL_CRLF);
	return EAGAIN;
    }
    memset(ext, 0x0, sizeof(omp_parallelfor_ext_t));

    if (NULL == attr_p) {
	ext->attr.flags = 0;
    } else {
	ext->attr = *attr_p;
    }

    ext->job_func = func;

    if (!ext->attr.f.is_uninterlocked) {
	    s3::mtxx_pthread_once_exec(omp_parallelfor_once,
				  omp_parallelfor_once_init);
    }

    ext->num_threads = (size_t)num_threads;
    ext->num_baris = ext->num_threads + 1;
    self_p->ext = ext;

    if (ext->attr.f.is_uninterlocked) {
	result = s3::mtxx_pthread_mutex_init(&ext->mutex, NULL);
	if (result) {
	    DBMS1(stderr,
		  "s3::mtxx_omp_parallelfor_init : s3::mtxx_pthread_mutex_init fail" EOL_CRLF);
	    status = EAGAIN;
	    goto out;
	} else {
	    ext->init.f.mutex = 1;
	}
    }

    result = mpxx_stl_vector_init( &ext->wth, sizeof(parallelfor_worker_t));
    if(result) {
	DBMS1(stderr, "s3::mtxx_omp_parallelfor_init : s3::mtxx_stl_vector_init(wth) fail, strerror:%s" EOL_CRLF, strerror(result));
	status = EAGAIN;
	goto out;
    } else {
	ext->init.f.wth = 1;
    }

    result = mpxx_stl_vector_reserve( &ext->wth, ext->num_threads);
    if(result) {
	DBMS1( stderr, "s3::mtxx_omp_parallelfor_init : s3::mtxx_stl_vector_reserve(wth) fail, strerror:%s" EOL_CRLF, strerror(result));
	status = EAGAIN;
	goto out;
    }

    result = s3::mtxx_sem_init(&ext->st_sem, 0, 0);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_parallelfor_init :  s3::mtxx_sem_init(st_sem) fail" EOL_CRLF);
	status = EAGAIN;
	goto out;
    } else {
	ext->init.f.st_sem = 1;
    }

    result =
	s3::mtxx_pthread_barrier_init(&ext->end_bari, NULL, (unsigned int)ext->num_baris);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_parallelfor_init :  s3::mtxx_pthread_barrier_ini(end_bari) fail" EOL_CRLF);
	status = -1;
	goto out;
    } else {
	ext->init.f.end_bari = 1;
    }

    for (n = 0; n < ext->num_threads; ++n) {
	parallelfor_worker_t t, *tp;

	memset( &t, 0x0, sizeof(t));
	t.no = n;
	t.ext_p = (void *) ext;

   	result = mpxx_stl_vector_push_back( &ext->wth, &t, sizeof(t));
	if(result) {
	    DBMS1( stderr, "s3::mtxx_omp_parallelfor_init : mpxx_stl_vector_push_back fail, strerror:%s" EOL_CRLF, strerror(result));
	    status = EAGAIN;
	    goto out;
	}

	tp = MPXX_STATIC_CAST(parallelfor_worker_t*, mpxx_stl_vector_ptr_at(&ext->wth, n));
	if(result) {
	    IFDBG1THEN {
		const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
		char xtoa_buf[MPXX_XTOA_DEC64_BUFSZ];

		mpxx_u64toadec( (uint64_t)n, xtoa_buf, xtoa_bufsz);
		DBMS1( stderr, "s3::mtxx_omp_parallelfor_init : mpxx_stl_vector_ptr_at(wth[%s] fail, strerror:%s" EOL_CRLF,
		    xtoa_buf, strerror(result));
	    }
	    status = -1;
	    goto out;
	}

	result =
	    s3::mtxx_pthread_create(&tp->pth, NULL, th_parallelfor_static_worker,
				   tp);
	if (result) {
	    IFDBG1THEN {
		const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
		char xtoa_buf[MPXX_XTOA_DEC64_BUFSZ];

		mpxx_u64toadec( (uint64_t)n, xtoa_buf, xtoa_bufsz);
		DBMS1(stderr,
		    "s3::mtxx_omp_parallelfor_init : pthread_create(th_parallelfor_static_worker:[%s) fail, strerror:%s" EOL_CRLF,
		      xtoa_buf, strerror(result));
	    }
	    status = EAGAIN;
	    goto out;
	} else {
	    tp->init.f.pth = 1;
	}
    }

    status = 0;
  out:
    if (status) {
	    s3::mtxx_omp_parallelfor_destroy(self_p);
    }

    return status;
}

/**
 * @fn int s3::mtxx_omp_parallelfor_destroy(s3::mtxx_omp_parallelfor_t *self_p)
 * @brief s3::mtxx_omp_parallelforオブジェクトを破棄します
 * @param self_p s3::mtxx_omp_parallelfor_tインスタンスポインタ
 * @retval -1 失敗
 * @retval 0 成功
 */
int s3::mtxx_omp_parallelfor_destroy(s3::mtxx_omp_parallelfor_t * self_p)
{
    omp_parallelfor_ext_t *const ext =
	get_omp_parallelfor_ext(self_p->ext);
    size_t n;
    int result;

    /* スレッドを終了します */
    if (ext->init.f.st_sem) {
	ext->stat.f.finish = 1;
	if( ext->init.f.wth ) {
	    for (n = 0; n < mpxx_stl_vector_get_pool_cnt(&ext->wth); ++n) {
		result = s3::mtxx_sem_post(&ext->st_sem);
		if (result) {
		    DBMS1(stderr,
		      "s3::mtxx_omp_parallelfor_destroy : s3::mtxx_sem_post(st_sem) fail" EOL_CRLF);
		    return -1;
		}
	    }
	    for (n = 0; n < mpxx_stl_vector_get_pool_cnt(&ext->wth); ++n) {
		parallelfor_worker_t *p = MPXX_STATIC_CAST(parallelfor_worker_t*,mpxx_stl_vector_ptr_at(&ext->wth,n));
		if (p->init.f.pth) {
		    result = s3::mtxx_pthread_join(&p->pth, NULL);
		    if (result) {
			IFDBG1THEN {
			    const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
			    char xtoa_buf[MPXX_XTOA_DEC64_BUFSZ];

			    mpxx_u64toadec( (uint64_t)n, xtoa_buf, xtoa_bufsz); 
    			    DBMS1(stderr,
			  "s3::mtxx_omp_parallelfor_destroy : s3::mtxx_pthread_join(pth:%s) fail" EOL_CRLF, xtoa_buf);
			}  
		    } else {
			p->init.f.pth = 0;
		    }
		}
		if (p->init.flags) {
		    IFDBG1THEN {
			const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
			char xtoa_buf_a[MPXX_XTOA_DEC64_BUFSZ];
			char xtoa_buf_b[MPXX_XTOA_DEC64_BUFSZ];

			mpxx_u64toadec( (uint64_t)p->no, xtoa_buf_a, xtoa_bufsz);
			mpxx_u64toahex( (uint64_t)p->init.flags, xtoa_buf_b, xtoa_bufsz, NULL);
			DBMS1(stderr,
			      "s3::mtxx_omp_parallelfor_destroy : wths[%s].init.flags=0x%s" EOL_CRLF, xtoa_buf_a, xtoa_buf_b);
		    }
		    return -1;
		}
	    }
	    result = mpxx_stl_vector_destroy(&ext->wth);
	    if(result) {
		DBMS1( stderr, "s3::mtxx_omp_parallelfor_destroy : mpxx_stl_vector_destory fail, strerror:%s" EOL_CRLF, strerror(result));
		return -1;
	    } else {
		ext->init.f.wth = 0;
	    }
	}

	result = s3::mtxx_sem_destroy(&ext->st_sem);
	if (result) {
	    DBMS1(stderr,
		  "s3::mtxx_omp_parallelfor_destroy : s3::mtxx_sem_destroy(st_sem) fail" EOL_CRLF);
	    return -1;
	} else {
	    ext->init.f.st_sem = 0;
	}
    }

    if (ext->init.f.end_bari) {
	result = s3::mtxx_pthread_barrier_destroy(&ext->end_bari);
	if (result) {
	    DBMS1(stderr,
		  "s3::mtxx_omp_parallelfor_destroy : s3::mtxx_pthread_barrier_destroy(end_bari) fail" EOL_CRLF);
	    return -1;
	} else {
	    ext->init.f.end_bari = 0;
	}
    }

    if (ext->init.f.mutex) {
	result = s3::mtxx_pthread_mutex_destroy(&ext->mutex);
	if (result) {
	    DBMS1(stderr,
		  "s3::mtxx_omp_parallelfor_destroy : s3::mtxx_pthread_mutex_destroy fail" EOL_CRLF);
	    return -1;
	} else {
	    ext->init.f.mutex = 0;
	}
    }

    if (ext->init.flags) {
	DBMS1(stderr,
	      "s3::mtxx_omp_parallelfor_destroy : init.flags=0x%llx" EOL_CRLF,
	      (long long)ext->init.flags);
	return -1;
    }
    free(ext);
    self_p->ext = NULL;

    return 0;
}

/**
 * @fn static int omp_parallelfor_waitforjob_exec(omp_parallelfor_ext_t * const __restrict ext)
 * @brief ワーカースレッドの全ての処理が終了するのを待ちます
 *	s3::mtxx_omp_parallelfor_attr_tのis_externalwaitforjob :並列処理実行終了を有効にしたとき使用します。 
 * @param ext omp_parallelfor_ext_tポインタ(インスタンス内部パラメータ)
 * @retval -1 失敗
 * @retval 0 成功
 */
static int omp_parallelfor_waitforjob_exec(omp_parallelfor_ext_t *
					   const __restrict ext)
{
    int result;

    result = s3::mtxx_pthread_barrier_wait(&ext->end_bari);
    if (result) {
	DBMS1(stderr,
	      "omp_parallelfor_waitforjob_exec : s3::mtxx_pthread_barrier_wait(end_bari) fail" EOL_CRLF);
    }

    if (ext->attr.f.is_uninterlocked) {
	result = s3::mtxx_pthread_mutex_unlock(&ext->mutex);
	if (result) {
	    DBMS1(stderr,
		  "omp_parallelfor_waitforjob_exec : s3::mtxx_pthread_mutex_unlock fail" EOL_CRLF);
	    return -1;
	}
    } else {
	omp_parallelfor_stat_t *const __restrict ss =
	    get_omp_parallelfor_stat();

	result = s3::mtxx_pthread_mutex_unlock(&ss->intelocked_mutex);
	if (result) {
	    DBMS1(stderr,
		  "omp_parallelfor_waitforjob_exec : s3::mtxx_pthread_mutex_unlock(intelocked_mutex) fail" EOL_CRLF);
	    return -1;
	}
    }

    return 0;
}

/**
 * @fn int s3::mtxx_omp_parallelfor_job_start( s3::mtxx_omp_parallelfor_t *self_p, void * const args, int duration)
 * @brief 作業を開始します。
 * @param self_p s3::mtxx_omp_parallelfor_tインスタンスポインタ
 * @param args 作業関数が参照するパラメータの引数ポインタ。
 *	※ 複数のスレッドから同一の引数ポインタを参照します。よって排他制御が必要な場合は独自に実装してください。
 * @param duration 継続数。スレッド単位に分割してスレッドに渡されます。
 * @retval 0 成功
 * @retval -1 失敗(処理が不安定になっています)
 */
int s3::mtxx_omp_parallelfor_job_start(s3::mtxx_omp_parallelfor_t * self_p,
				      void *args, size_t duration)
{
    int result;
    omp_parallelfor_ext_t *const ext =
	get_omp_parallelfor_ext(self_p->ext);
    size_t n;
    size_t start_pos, procs, remain;
    omp_parallelfor_stat_t *const __restrict ss =
	get_omp_parallelfor_stat();

    if (!(ext->attr.f.is_uninterlocked)) {
	result = s3::mtxx_pthread_mutex_lock(&ss->intelocked_mutex);
	if (result) {
	    DBMS1(stderr,
		  "s3::mtxx_omp_parallelfor_job_start : s3::mtxx_pthread_mutex_lock(intelocked_mutex) fail" EOL_CRLF);
	    return -1;
	}
    } else {
	result = s3::mtxx_pthread_mutex_lock(&ext->mutex);
	if (result) {
	    DBMS1(stderr,
		  "s3::mtxx_omp_parallelfor_job_start : s3::mtxx_pthread_mutex_lock(ext) fail" EOL_CRLF);
	    return -1;
	}
    }

    procs = duration / ext->num_threads;
    if (duration % ext->num_threads) {
	procs++;
    }

    ext->func_args = args;

    /* スレッドごとのduration設定 */
    for ( n = 0, start_pos = 0, remain = duration; n < ext->num_threads;
	 ++n) {
	parallelfor_worker_t *t = MPXX_STATIC_CAST(parallelfor_worker_t*,mpxx_stl_vector_ptr_at(&ext->wth,n));
	t->start_pos = start_pos;
	t->duration = (procs < remain) ? procs : remain;
	start_pos += t->duration;
	remain -= t->duration;
    }

    /* スレッドスタート */
    for (n = 0; n < ext->num_threads; ++n) {
	result = s3::mtxx_sem_post(&ext->st_sem);
	if (result) {
	    IFDBG1THEN {
		const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
		char xtoa_buf[MPXX_XTOA_DEC64_BUFSZ];

		mpxx_u64toadec( (uint64_t)n, xtoa_buf, xtoa_bufsz); 
		DBMS1(stderr,
		  "s3::mtxx_omp_parallelfor_job_start : s3::mtxx_sem_post(%s) fail" EOL_CRLF, xtoa_buf);
	    }
	}
    }

    /* フラグが0なら処理終了待ち */
    if (!ext->attr.f.is_externalwaitforjob) {
	result = omp_parallelfor_waitforjob_exec(ext);
	if (result) {
	    DBMS1(stderr,
		  "s3::mtxx_omp_parallelfor_job_start : omp_parallelfor_waitforjob_exec fail" EOL_CRLF);
	    return -1;
	}
    }

    return 0;
}


/**
 * @fn int s3::mtxx_omp_parallelfor_waitforjob(s3::mtxx_omp_parallelfor_t *self_p)
 * @brief 並列化した全てのスレッド処理が終了するのを待ちます
 * @param self_p s3::mtxx_omp_parallelfor_tインスタンスポインタ
 * @retval -1 失敗
 * @retval 0 処理終了
 */
int s3::mtxx_omp_parallelfor_waitforjob(s3::mtxx_omp_parallelfor_t * self_p)
{
    omp_parallelfor_ext_t *ext = 
	get_omp_parallelfor_ext(self_p->ext);
    int result;

    result = omp_parallelfor_waitforjob_exec(ext);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_parallelfor_waitforjob : omp_parallelfor_waitforjob_exec fail" EOL_CRLF);
	return -1;
    }

    return 0;
}

/**
 * @fn int s3::mtxx_omp_parallelfor_get_num_threads(s3::mtxx_omp_parallelfor_t *self_p)
 * @brief インスタンス内に生成されたスレッド数を返します。
 * @param self_p s3::mtxx_omp_parallelfor_tインスタンスポインタ
 * @retval -1 失敗
 * @retval 1以上 スレッド数
 */
int s3::mtxx_omp_parallelfor_get_num_threads(s3::mtxx_omp_parallelfor_t *
					    self_p)
{
    omp_parallelfor_ext_t *ext = 
	get_omp_parallelfor_ext(self_p->ext);

    return (ext->num_threads == 0) ? -1 : (int)ext->num_threads;
}

/**
 * @fn int s3::mtxx_omp_parallelfor_simply_exec(s3::mtxx_omp_parallelfor_callback_t func, void *args, int duration)
 * @brief funcで指定された関数を有効なコア数に分割して並列処理させます。
 *	s3::mtxx_omp_parallelforを一連の動作を一つの関数で実行します。よって、オーバヘッドがあります。
 *	※ このから呼ばれるコールバック関数中から再度このこの関数を呼び出すことはしないでください。デッドロックします。
 *	もし、そのような使い方をしたいならば is_uninterlockedフラグを有効にして s3::mtxx_omp_parallelfor_init()からコーディングしてください。
 * @param func スレッドから呼び出されるコールバック関数ポインタ
 * @param args funcに引き渡される引数ポインタ
 * @param duration 継続数(起動したスレッド数に分割されfuncの引数として渡ります。
 * @retval -1 失敗
 * @retval 0 成功
 */
int s3::mtxx_omp_parallelfor_simply_exec(s3::mtxx_omp_parallelfor_callback_t
					func, void *args, size_t duration)
{
	s3::mtxx_omp_parallelfor_t self;
    int result, status;

    result = s3::mtxx_omp_parallelfor_init(&self, func, 0, NULL);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_parallelfor_simply_exec : s3::mtxx_omp_parallelfor_init fail" EOL_CRLF);
	return -1;
    }

    result = s3::mtxx_omp_parallelfor_job_start(&self, args, duration);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_parallelfor_simply_exec :  s3::mtxx_omp_parallelfor_job_star fail" EOL_CRLF);
	status = -1;
	goto out;
    }

    status = 0;

  out:
    result = s3::mtxx_omp_parallelfor_destroy(&self);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_parallelfor_simply_exec : s3::mtxx_omp_parallelfor_destroy fail" EOL_CRLF);
	return -1;
    }

    return status;
}

/**
 * @fn int s3::mtxx_omp_get_num_procs(void)
 * @brief 利用可能な CPU数を返します
 */
int s3::mtxx_omp_get_num_procs(void)
{
    int64_t num_procs;

    num_procs = mpxx_sysconf(MPXX_SYSCNF_NPROCESSORS_ONLN);
    if (num_procs <= 0) {
	DBMS1(stderr,
	      "s3::mtxx_omp_get_num_procs : mpxx_sysconf(MPXX_SYSCNF_NPROCESSORS_ONLN) fail" EOL_CRLF);
	return -1;
    }

    return (int) num_procs;
}
