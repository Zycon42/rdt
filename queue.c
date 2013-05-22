/**
 * Projekt c.3 do predmetu IPK
 * @author Jan Dusek <xdusek17@stud.fit.vutbr.cz>
 * @date 20.4.2011
 */

#include "queue.h"

void queue_init(queue_t* q, delete_item_func_t func) {
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    q->del_func = func;
}

int queue_append(queue_t* q, const queue_data_t* data) {
    queue_item_t *item = malloc(sizeof(queue_item_t));
    if (item == NULL)
        return 0;

    item->qdata = *data;
    item->next = NULL;

    if (q->head == NULL)
        q->head = item;
    else
        q->tail->next = item;

    q->tail = item;

    return ++q->size;
}

int queue_is_empty(queue_t* q) {
    return !q->size;
}

void queue_remove(queue_t* q) {
    q->del_func(q->head->qdata.data);
    queue_item_t *oldhead = q->head;
    q->head = q->head->next;
    free(oldhead);
    q->size--;
}

void queue_destroy(queue_t* q) {
    while (!queue_is_empty(q))
        queue_remove(q);
}

queue_data_t* queue_get(queue_t* q, size_t index) {
    int i = 0;
    queue_item_t *item = q->head;
    while (item != NULL) {
        if (i++ == index)
            return &item->qdata;

        item = item->next;
    }
    return NULL;
}

