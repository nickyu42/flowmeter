#pragma once

#include "CircularBuffer.h"
#include "mbed.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>

template <uint32_t BufferSize> class CharCircularBuffer {

public:
  CharCircularBuffer() : _total(0), _mean(0), _tail(0), _head(0) {}

  ~CharCircularBuffer() {}

  void push(const uint16_t &data) {
    core_util_critical_section_enter();

    this->_buffer[this->_head] = data;

    this->_tail = this->_head;
    this->_head = incrementCounter(this->_head);

    this->_total -= this->_buffer[this->_tail];
    this->_total += this->_buffer[this->_head];
    this->_mean = static_cast<float>(this->_total) / BufferSize;

    core_util_critical_section_exit();
  }

  uint32_t incrementCounter(uint32_t val) {
    val++;

    if (val == BufferSize) {
      val = 0;
    }

    return val;
  }

  float mean() { return this->_mean; }

private:
  int64_t _total;
  float _mean;

  int16_t _buffer[BufferSize];
  uint32_t _tail;
  uint32_t _head;
};