/*
 * queue.h
 *
 *  Created on: Nov 3, 2024
 *      Author: vojta
 */

#ifndef USER_QUEUE_H_
#define USER_QUEUE_H_

#include <stdint.h>

typedef struct QueueT {
	uint8_t begin;
	uint8_t end;
	uint8_t data[256];
} Queue;

#define QueueInit(q) do { (q)->begin = (q)->end = 0; } while (0)
#define QueueNotEmpty(q) ((q)->begin != (q)->end)
#define QueueGet(q) ((q)->data[(q)->begin++])
#define QueuePut(q, d) do { (q)->data[(q)->end++] = (d); } while (0)

#endif /* USER_QUEUE_H_ */
