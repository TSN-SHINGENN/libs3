#ifndef INC_MTXX_OMP_SINGLE_H
#define INC_MTXX_OMP_SINGLE_H

namespace s3 {

typedef struct _mtxx_omp_single {
void *ext;
} mtxx_omp_single_t;

typedef struct _mtxx_omp_single_attr {
    unsigned int flags;
    struct {
	unsigned int is_nowait:1; /* 同期せずにそのまま続行します */
    } f;
} mtxx_omp_single_attr_t;

typedef void (*mtxx_omp_single_callback_t)(void *args);

int mtxx_omp_single_get_instance( mtxx_omp_single_t *self_p, mtxx_omp_single_attr_t *attr);
int mtxx_omp_single_exec( mtxx_omp_single_t *self_p, mtxx_omp_single_callback_t func, void *args);
int mtxx_omp_single_reset_instance( mtxx_omp_single_t *self_p);
int mtxx_omp_single_detach_instance( mtxx_omp_single_t *self_p);
int mtxx_omp_single_get_number_of_remaining_instance(int *max_p);

}


#include <functional>

typedef std::function<void(void*)> mtxx_omp_single_callback_t;

class cmtxx_omp_single 
{
private:
    s3::mtxx_omp_single_t obj;
    s3::mtxx_omp_single_callback_t callback_function;

public:
    cmtxx_omp_single();
    cmtxx_omp_single(s3::mtxx_omp_single_attr_t *attr_p);
    ~cmtxx_omp_single();

    int single_exec(mtxx_omp_single_callback_t func, void *args);

    template<class CLASS_T>
    int single_exec(void (CLASS_T::*func)(void*), CLASS_T &owner, void *args);
    int single_reset_instance();
};

#endif /* end of INC_MTXX_OMP_SINGLE_H */
