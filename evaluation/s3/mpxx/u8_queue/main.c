#include <stdio.h>

#define QUEUE_SIZE 7

struct queue_data {
    int queue[QUEUE_SIZE];
    int head; /* �L���[�̐擪�ʒu */
    int tail; /* �L���[�̖����ʒu */
};
typedef struct queue_data queue_t;

/* �L���[�̎��̑}���ʒu�����߂� */
#define queue_next(n) (((n) + 1) % QUEUE_SIZE)

/*!
 * @brief         �L���[�̏���������
 * @param[in/out] q �L���[�Ǘ�
 */
static void
queue_init(queue_t* q)
{
    q->head = 0;
    q->tail = 0;
}

/*!
 * @brief     �L���[����ł��邩���ׂ�B
 * @param[in] q     �L���[�Ǘ�
 * @return    ��Ȃ��1��Ԃ��A��łȂ����0��Ԃ��B
 */
int
queue_isempty(queue_t q)
{
    return(q.head == q.tail);
}

/*!
 * @brief     �L���[�Ƀf�[�^��}������
 * @param[in] q     �L���[�Ǘ�
 * @param[in] data  �}������f�[�^
 * @return    0     success
 * @return    -1    failure
 */
int
queue_push(queue_t* q, int data)
{
    /* �L���[�����t�ł��邩�m�F���� */
    if (queue_next(q->tail) == q->head) return(-1);

    /* �L���[�̖����Ƀf�[�^��}������ */
    q->queue[q->tail] = data;

    /* �L���[�̎���}���ʒu������ɂ��� */
    q->tail = queue_next(q->tail);

    return(0);
}

/*!
 * @brief      �L���[����f�[�^�����o��
 * @param[in]  q     �L���[�Ǘ�
 * @param[out] data  �f�[�^
 * @return     0     success
 * @return     -1    failure
 */
int
queue_pop(queue_t* q, int* data)
{
    /* �L���[�Ɏ��o���f�[�^�����݂��邩�m�F���� */
    if (q->head == q->tail) return(-1);

    /* �L���[����f�[�^���擾���� */
    *data = q->queue[q->head];

    /* ���̃f�[�^�擾�ʒu�����肷�� */
    q->head = queue_next(q->head);

    return(0);
}

/*!
 * @brief     �z��v�f�ʒu���҂��s��͈͂ł��邩�m�F����
 * @param[in] q   �L���[�Ǘ�
 * @param[in] id  �z��̗v�f�ʒu
 */
static int
is_use_range(queue_t* q, int id)
{
    if ((id < q->tail) && (id >= q->head)) return(1);
    if ((id > q->tail) && (id >= q->head) && (q->head > q->tail)) return(1);
    if ((id < q->tail) && (id < q->head) && (q->head > q->tail)) return(1);

    return(0);
}

/*!
 * @brief      �L���[�̗v�f���ꗗ�\������
 * @param[in]  q     �L���[�Ǘ�
 */
static void
queue_print(queue_t* q)
{
    int cnt;

    printf("queue [");
    for (cnt = 0; cnt < QUEUE_SIZE; cnt++) {
        if (is_use_range(q, cnt)) {
            printf("%2d", q->queue[cnt]);
        }
        else {
            printf("%2c", '.');
        }
    }
    printf("]\n");
}

int
main(void)
{
    queue_t q;
    int data = 0;
    int cnt;

    queue_init(&q);

    /* �L���[�Ƀf�[�^����͂��� */
    for (cnt = 1; cnt < 5; cnt++) {
        printf("enqueue %d : ", cnt);
        if (queue_push(&q, cnt) != 0) {
            printf("Queue is full.\n");
        }
        else {
            queue_print(&q);
        }
    }

    /* �L���[����f�[�^���擾���� */
    for (cnt = 1; cnt < 4; cnt++) {
        if (queue_pop(&q, &data) != 0) {
            printf("Queue is empty.\n");
        }
        else {
            printf("dequeue %d : ", data);
            queue_print(&q);
        }
    }

    /* �L���[�Ƀf�[�^����͂��� */
    for (cnt = 5; cnt < 10; cnt++) {
        printf("enqueue %d : ", cnt);
        if (queue_push(&q, cnt) != 0) {
            printf("Queue is full.\n");
        }
        else {
            queue_print(&q);
        }
    }

    /* �L���[����f�[�^���擾���� */
    for (cnt = 1; cnt < 4; cnt++) {
        if (queue_pop(&q, &data) != 0) {
            printf("Queue is empty.\n");
        }
        else {
            printf("dequeue %d : ", data);
            queue_print(&q);
        }
    }

    return(0);
}
