#pragma once

#include "CircularBuffer.h"
#include "mbed.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>

template <uint32_t BufferSize> class CharCircularBuffer {

public:
  CharCircularBuffer() : _total(0), _mean(0), _tail(1), _head(0) {}

  ~CharCircularBuffer() {}

  void push(const int16_t &data) {
    core_util_critical_section_enter();

    this->_tail = incrementCounter(this->_tail);
    this->_head = incrementCounter(this->_head);

    this->_buffer[this->_head] = data;

    this->_total -= static_cast<int64_t>(this->_buffer[this->_tail]);
    this->_total += static_cast<int64_t>(this->_buffer[this->_head]);

    this->_mean =
        static_cast<double>(this->_total) / static_cast<double>(BufferSize);

    core_util_critical_section_exit();
  }

  uint32_t incrementCounter(uint32_t val) {
    val++;

    if (val == BufferSize) {
      val = 0;
    }

    return val;
  }

  double mean() { return this->_mean; }

private:
  int64_t _total;
  double _mean;

  int16_t _buffer[BufferSize];
  uint32_t _tail;
  uint32_t _head;
};