#ifndef INC_MTXX_OMP_PARALLELFOR_H
#define INC_MTXX_OMP_PARALLELFOR_H

#pragma once
#include <stddef.h>

namespace s3 {

/**
 * @var typedef void (*mtxx_omp_parallelfor_callback_t)(int thread_no, void *args, size_t start_pos, size_t duration)
 * @brief mtxx_omp_parallelforのスレッドから呼び出される関数宣言
 * @param args mtxx_omp_parallelfor_job_start()で引き渡される引数ポインタ
 * @param start_pos mtxx_omp_parallelfor_job_start()のduration値からスレッド数で計算されたstart_pos値
 * @param duration mtxx_omp_parallelfor_job_start()のduration値からスレッド数で計算されたスレッドで処理されるべきduration値
 */
typedef void (*mtxx_omp_parallelfor_callback_t)(int thread_no, void *args, size_t start_pos, size_t duration);

/**
 * @typedef mtxx_omp_parallelfor_attr_t
 * @brief mtxx_omp_parallelforの属性フラグ集合を設定するときに使うunion共同体です
 */
typedef union _mtxx_omp_parallelfor_attr {
    unsigned int flags; /*!< 主にフラグ群を0クリアするときに使用します */
    struct { 
	unsigned int schedule_is_dynamic:1; /*! スケジューリングをDYMAMIC(指定がなければチャンクサイズ１）に設定します。0はSTATIC */
	unsigned int is_uninterlocked:1; /*!< このインスタンスが全てのインスタンスと排他的に処理されないようにします */
	unsigned int is_externalwaitforjob:1; /*!< 並列処理実行終了を mtxx_omp_parallelfor_waitforjob()で待つようにします */
	unsigned int is_called_even0duration:1; /*!< durationが0だったとしてもコールバック関数を呼び出します。 */
    } f; /*!< 属性フラグ集合です */
} mtxx_omp_parallelfor_attr_t;

/**
 * @typedef mtxx_omp_parallelfor_t
 * @brief mtxx_omp_parallelforのインスタンスを管理する構造体です。
 */
typedef struct _mtxx_omp_parallelfor {
    void *ext; /*! 内部管理情報のポインタが保存されます */
} mtxx_omp_parallelfor_t;

int mtxx_omp_parallelfor_init(mtxx_omp_parallelfor_t *self_p, mtxx_omp_parallelfor_callback_t func, size_t num_threads, mtxx_omp_parallelfor_attr_t *attr);
int mtxx_omp_parallelfor_job_start( mtxx_omp_parallelfor_t *self_p, void * const args, size_t duration);
int mtxx_omp_parallelfor_waitforjob(mtxx_omp_parallelfor_t *self_p);
int mtxx_omp_parallelfor_destroy(mtxx_omp_parallelfor_t *self_p);
int mtxx_omp_parallelfor_get_num_threads(mtxx_omp_parallelfor_t *self_p);

int mtxx_omp_parallelfor_simply_exec(mtxx_omp_parallelfor_callback_t func, void *args, size_t duration);

int mtxx_omp_get_num_procs(void);

int mtxx_omp_parallelfor_static_init(mtxx_omp_parallelfor_t *self_p, mtxx_omp_parallelfor_callback_t func, int num_threads, mtxx_omp_parallelfor_attr_t *attr);

int mtxx_omp_parallelfor_dynamic_init(mtxx_omp_parallelfor_t *self_p, mtxx_omp_parallelfor_callback_t func, size_t chunk_size, int num_thread, mtxx_omp_parallelfor_attr_t *attr);

}

#if defined (__cplusplus )
#define MULTIOSXX_OMP_PARALLELFOR

#include <functional>

namespace s3 {

typedef std::function<void(int, void*, size_t, size_t)> mtxx_omp_parallelfor_cbfunc_t;

template <class CLASS_T>
class cmtxx_omp_parallelfor
{
private:
    mtxx_omp_parallelfor_t obj;
    mtxx_omp_parallelfor_cbfunc_t callback_function;
public:
    cmtxx_omp_parallelfor(mtxx_omp_parallelfor_cbfunc_t func_ptr);
    cmtxx_omp_parallelfor(mtxx_omp_parallelfor_cbfunc_t func_ptr, int num_threads, mtxx_omp_parallelfor_attr_t *attr_p);

    cmtxx_omp_parallelfor(void (CLASS_T::*func)(int, void*, size_t, size_t), CLASS_T &owner, int num_threads, mtxx_omp_parallelfor_attr_t *attr_p);

    cmtxx_omp_parallelfor(void (CLASS_T::*func)(int, void*, size_t, size_t), CLASS_T &owner);
    
    virtual ~cmtxx_omp_parallelfor();

    int parallelfor_job_start(void * const args, size_t duration);
    int parallelfor_waitforjob(void);

    int parallelfor_get_num_threads(void);
    int get_number_of_CPUS(void);

};

}

#if 0
template <class CLASS_T>
multiosxx_omp_parallelfor_stdfunc_t multiosxx_omp_parallelfor_class_member2stdfunc(void (CLASS_T::*func)(int, void*, size_t, size_t), CLASS_T &owner);
#endif

#endif /* end of MULTIOSXX_OMP_PARALLELFOR */

#endif /* end of INC_MTXX_OMP_PARALLELFOR_H */
