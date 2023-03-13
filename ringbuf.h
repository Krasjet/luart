/* ringbuf.h: ringbuffer for plotting */
#pragma once

#include <vector>
#include <cstddef>

template<typename T>
struct RingBuf {
  std::vector<T> buf;
  size_t size;
  size_t wp;

  explicit RingBuf(size_t size) : buf(size), size(size), wp(0) {}

  void push(const T* x, size_t n)
  {
    for (size_t i = 0; i < n; i++)
      push(x[i]);
  }

  inline void push(const T x)
  {
    buf[wp] = x;
    size_t wp_new = wp + 1;
    if (wp_new >= size)
      wp_new -= size;
    wp = wp_new;
  }

  // get latest sample
  const T peek() const
  {
    if (wp == 0)
      return buf[size-1];
    return buf[wp-1];
  }
};
