#include "fifo.h"

fifo* create_fifo(size_t capacity)
{
    fifo* fp = (fifo*) malloc(sizeof(fifo));
    if(fp)
    {
        fp->size = 0;
        fp->capacity = capacity;
        fp->first = 0;
        fp->last = -1;
        fp->elems = (fifo_elem*) malloc(capacity * sizeof(fifo_elem));
        if(NULL == fp->elems)
            return NULL;
    }
    return fp;
}

int enqueue(fifo* fp, fifo_elem el)
{
    if(fp->size+1 > fp->capacity)  // no space left
        return -1;

    fp->size++;

    if(fp->last < (fp->capacity - 1)) // establish the end of the fifo
        fp->last++;
    else
        fp->last = 0;

    fp->elems[fp->last] = el;
    
    return 0;
}

int dequeue(fifo* fp, fifo_elem* el)
{
    if (0 == fp->size) // no elems
        return -1;

    *el = fp->elems[fp->first];
    fp->size--;

    if (fp->first < (fp->capacity - 1)) // establish the begin of the fifo
        fp->first++;
    else
        fp->first = 0;

    return 0;
}

void destroy_fifo(fifo* fp)
{
    if(fp)
    {
        if(fp->elems)
            free(fp->elems);
        free(fp);
    }
}
