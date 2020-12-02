#ifndef INC_MTXX_ACTOR_SIMPLE_MPI_H
#define INC_MTXX_ACTOR_SIMPLE_MPI_H

#pragma once

namespace s3 {

typedef enum _enum_mtxx_actor_simple_mpi_block {
    MTXX_ACTOR_SIMPLE_MPI_NOBLOCK = 10,
    MTXX_ACTOR_SIMPLE_MPI_BLOCK,
    MTXX_ACTOR_SIMPLE_MPI_NOBLOCKWATCH
} enum_mtxx_actor_simple_mpi_block_t;

typedef enum _enum_mtxx_actor_simple_mpi_res {
    MTXX_ACTOR_SIMPLE_MPI_RES_NACK = 200,
    MTXX_ACTOR_SIMPLE_MPI_RES_ACK
} enum_mtxx_actor_simple_mpi_res_t;

typedef struct _mtxx_actor_simple_mpi {
    int req_cmd;
    enum_mtxx_actor_simple_mpi_res_t res;
    void *ext;
} mtxx_actor_simple_mpi_t;

    int mtxx_actor_simple_mpi_init(mtxx_actor_simple_mpi_t *const self_p);
    int mtxx_actor_simple_mpi_destroy(mtxx_actor_simple_mpi_t *const self_p);
    int mtxx_actor_simple_mpi_send_request(mtxx_actor_simple_mpi_t *const self_p, const int cmd, enum_mtxx_actor_simple_mpi_res_t *const res_p);
    int mtxx_actor_simple_mpi_recv_request(mtxx_actor_simple_mpi_t *const self_p, int *const cmd_p, enum_mtxx_actor_simple_mpi_block_t block_type);
    int mtxx_actor_simple_mpi_reply(mtxx_actor_simple_mpi_t *const self_p, enum_mtxx_actor_simple_mpi_res_t res);

}

#endif /* end of INC_MTXX_ACTOR_SIMPLE_MPI_H */
