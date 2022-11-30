#ifndef POWER_BUFFER_H
#define POWER_BUFFER_H

#include <stdint.h>

#define BUFFER_SIZE 100

int16_t _buffer[BUFFER_SIZE];
uint32_t _head = 0;
uint32_t _tail = 1;

double _mean = 0;
uint64_t _total = 0;

static uint32_t incrementCounter(uint32_t val)
{
    val++;

    if (val == BUFFER_SIZE)
    {
        val = 0;
    }

    return val;
}

void circ_buffer_push(const int16_t data)
{
    _buffer[_head] = data;

    _total -= (int64_t)_buffer[_tail];
    _total += (int64_t)_buffer[_head];

    _mean = (double)_total / (double)BUFFER_SIZE;
    
    _tail = incrementCounter(_tail);
    _head = incrementCounter(_head);
}

double circ_buffer_mean() { return _mean; }

#endif