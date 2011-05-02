/* 
 * File:   fifo.h
 * Author: adizere
 *
 * Created on March 30, 2011, 10:55 PM
 */

#ifndef _FIFO_H
#define	_FIFO_H

#include <stdlib.h>

typedef int fifo_elem; // TODO change to payload

typedef struct _fifo
{
    size_t size;
    size_t capacity;
    size_t first;
    size_t last;
    fifo_elem* elems;
} fifo;

fifo* create_fifo(size_t capacity);

int enqueue(fifo* fp, fifo_elem el);

int dequeue(fifo* fp, fifo_elem* el);

void destroy_fifo(fifo* fp);

#endif	/* _FIFO_H */

