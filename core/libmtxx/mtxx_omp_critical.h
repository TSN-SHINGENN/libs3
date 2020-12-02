#ifndef INC_MTXX_OMP_CRITICAL_H
#define INC_MTXX_OMP_CRITICAL_H

#include <stdint.h>

namespace s3 {

typedef struct _mtxx_omp_critical {
    uint64_t start_sequence_no;
    void *ext;
} mtxx_omp_critical_t;

typedef union _mtxx_omp_critical_attr {
    unsigned int flags;
    struct {
	unsigned int is_unrefer_sequence_no; /* crtical_startで指定されるseq_noを参照しない */
    } f;
} mtxx_omp_critical_attr_t;

int mtxx_omp_critical_init(mtxx_omp_critical_t *self_p, uint64_t start_seq_no, mtxx_omp_critical_attr_t *attr_p);
int mtxx_omp_critical_start(mtxx_omp_critical_t *self_p, uint64_t waiting_proc_no);
int mtxx_omp_critical_end(mtxx_omp_critical_t *self_p, uint64_t seq_no);
int mtxx_omp_critical_end_and_cancel_all( mtxx_omp_critical_t *self_p );
int mtxx_omp_critical_destroy( mtxx_omp_critical_t *self_p );
int mtxx_omp_critical_is_canceled( mtxx_omp_critical_t *self_p );
uint64_t mtxx_omp_critical_get_now_sequence_no( mtxx_omp_critical_t *self_p);
}

#endif /* INC_MTXX_OMP_CRITICAL_H */
