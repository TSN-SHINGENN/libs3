#include <stdio.h>

#define QUEUE_SIZE 7

struct queue_data {
    int queue[QUEUE_SIZE];
    int head; /* キューの先頭位置 */
    int tail; /* キューの末尾位置 */
};
typedef struct queue_data queue_t;

/* キューの次の挿入位置を求める */
#define queue_next(n) (((n) + 1) % QUEUE_SIZE)

/*!
 * @brief         キューの初期化処理
 * @param[in/out] q キュー管理
 */
static void
queue_init(queue_t* q)
{
    q->head = 0;
    q->tail = 0;
}

/*!
 * @brief     キューが空であるか調べる。
 * @param[in] q     キュー管理
 * @return    空ならば1を返し、空でなければ0を返す。
 */
int
queue_isempty(queue_t q)
{
    return(q.head == q.tail);
}

/*!
 * @brief     キューにデータを挿入する
 * @param[in] q     キュー管理
 * @param[in] data  挿入するデータ
 * @return    0     success
 * @return    -1    failure
 */
int
queue_push(queue_t* q, int data)
{
    /* キューが満杯であるか確認する */
    if (queue_next(q->tail) == q->head) return(-1);

    /* キューの末尾にデータを挿入する */
    q->queue[q->tail] = data;

    /* キューの次回挿入位置を決定にする */
    q->tail = queue_next(q->tail);

    return(0);
}

/*!
 * @brief      キューからデータを取り出す
 * @param[in]  q     キュー管理
 * @param[out] data  データ
 * @return     0     success
 * @return     -1    failure
 */
int
queue_pop(queue_t* q, int* data)
{
    /* キューに取り出すデータが存在するか確認する */
    if (q->head == q->tail) return(-1);

    /* キューからデータを取得する */
    *data = q->queue[q->head];

    /* 次のデータ取得位置を決定する */
    q->head = queue_next(q->head);

    return(0);
}

/*!
 * @brief     配列要素位置が待ち行列範囲であるか確認する
 * @param[in] q   キュー管理
 * @param[in] id  配列の要素位置
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
 * @brief      キューの要素を一覧表示する
 * @param[in]  q     キュー管理
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

    /* キューにデータを入力する */
    for (cnt = 1; cnt < 5; cnt++) {
        printf("enqueue %d : ", cnt);
        if (queue_push(&q, cnt) != 0) {
            printf("Queue is full.\n");
        }
        else {
            queue_print(&q);
        }
    }

    /* キューからデータを取得する */
    for (cnt = 1; cnt < 4; cnt++) {
        if (queue_pop(&q, &data) != 0) {
            printf("Queue is empty.\n");
        }
        else {
            printf("dequeue %d : ", data);
            queue_print(&q);
        }
    }

    /* キューにデータを入力する */
    for (cnt = 5; cnt < 10; cnt++) {
        printf("enqueue %d : ", cnt);
        if (queue_push(&q, cnt) != 0) {
            printf("Queue is full.\n");
        }
        else {
            queue_print(&q);
        }
    }

    /* キューからデータを取得する */
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
