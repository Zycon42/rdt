/**
 * Projekt c.3 do predmetu IPK
 * @author Jan Dusek <xdusek17@stud.fit.vutbr.cz>
 * @date 20.4.2011
 */

#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stdlib.h>

typedef void (*delete_item_func_t)(void *);

typedef struct {
    void *data;
    size_t datalen;
} queue_data_t;

/// item in queue
typedef struct _queue_item {
    /// data
    queue_data_t qdata;
    struct _queue_item *next;
} queue_item_t;

/// queue
typedef struct {
    /// queue front
    queue_item_t *head;
    /// queue back
    queue_item_t *tail;
    /// size of queue
    size_t size;
    delete_item_func_t del_func;
} queue_t;

/**
 * Initializes queue
 * @param q queue
 */
void queue_init(queue_t *q, delete_item_func_t func);

/**
 * Determines if is queue empty
 * @param q queue
 * @return 0 on false nonzero on success
 */
int queue_is_empty(queue_t *q);

/**
 * Appends data to back of queue
 * @param q queue
 * @param data data to add. shalow copy
 * @return 0 on error new size on success
 */
int queue_append(queue_t *q, const queue_data_t *data);

/**
 * Removes item data from queue head. Calls del_func on item data
 * Queue must NOT be empty
 * @param q queue
 */
void queue_remove(queue_t *q);

/**
 * Destroys whole queue. calls del_func on each item.
 * @param q queue
 */
void queue_destroy(queue_t *q);

/**
 * Gets item data from queue at index
 * @param q queue
 * @param index index must be in 0..q->size
 * @return item data at index
 */
queue_data_t * queue_get(queue_t *q, size_t index);

#endif // _QUEUE_H_
