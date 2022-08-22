#pragma once

#include <array>
#include <initializer_list>
#include <vector>

#include <cstddef>


template<typename T>
class span
{
public:
  span(T            *data_,
       const size_t  size_,
       const size_t  offset_ = 0)
    : _data(data_ + offset_),
      _size(size_)
  {
  }

  span(const span<T> &data_,
       const size_t   offset_ = 0)
    : span(data_.data(),
           data_.size(),
           offset_)
  {
  }

  span(std::vector<T> &vec_,
       const size_t    offset_ = 0)
    : span(vec_.data(),
           vec_.size(),
           offset_)
  {
  }

  template<size_t N>
  span(const std::array<T,N> &arr_,
       const size_t           offset_ = 0)
    : span(arr_.data(),
           arr_.size(),
           offset_)
  {
  }

public:
  span<T>&
  operator=(std::vector<T> &vec_)
  {
    _data = vec_.data();
    _size = vec_.size();
    return *this;
  }

public:
  T      *data() const { return _data; }
  size_t  size() const { return _size; }

public:
  T&
  operator[](const size_t i_)
  {
    return &_data[i_];
  }

  const
  T&
  operator[](const size_t i_) const
  {
    return _data[i_];
  }

public:
  T *begin() { return _data; }
  T* end() { return &_data[_size]; }
  const T *cbegin() const { return _data; }
  const T *cend() const { return &_data[_size]; }

private:
  T      *_data;
  size_t  _size;
};

template<typename T>
class cspan
{
public:
  cspan()
    : _data(nullptr),
      _size(0),
      _off(0)
  {
  }

  cspan(const T      *data_,
        const size_t  size_,
        const size_t  off_ = 0)
    : _data(data_),
      _size(size_),
      _off(off_)
  {
  }

  cspan(const T      *begin_,
        const T      *end_,
        const size_t  off_ = 0)
  {
    _data = begin_;
    _size = end_ - begin_;
    _off  = off_;
  }

  cspan(const cspan<T> &data_,
        const size_t    off_ = 0)
  {
    _data = data_._data;
    _size = data_._size - off_;
    _off  = data_._off + off_;
  }

  cspan(const std::vector<T> &vec_,
        const size_t          off_ = 0)
  {
    _data = vec_.data();
    _size = vec_.size() - off_;
    _off  = off_;
  }

  template<size_t N>
  cspan(const std::array<T,N> &arr_,
        const size_t           off_ = 0)
  {
    _data = arr_.data();
    _size = arr_.size() - off_;
    _off  = off_;
  }

public:
  cspan<T>&
  operator=(const std::vector<T> &vec_)
  {
    _data = vec_.data();
    _size = vec_.size();
    _off  = 0;
    return *this;
  }

public:
  const
  T&
  operator[](const size_t i_) const
  {
    return _data[_off + i_];
  }

public:
  operator bool() const
  {
    return ((_data != NULL && (_size != 0)));
  }

public:
  cspan<T>
  operator()(const size_t off_)
  {
    return cspan<T>(*this,off_);
  }

  cspan<T>
  operator()(const size_t off_start_,
             const size_t off_end_)
  {
    const size_t size = (off_end_ - off_start_);

    return cspan<T>(_data,
                    size,
                    _off + off_start_);
  }

public:
  const T *data() const { return &_data[_off]; }
  size_t   size() const { return _size; }

public:
  size_t off() const { return _off; }

public:
  const T *begin() const { return &_data[_off]; }
  const T *end() const { return begin() + _size; }
  const T *cbegin() const { return &_data[_off]; }
  const T *cend() const { return cbegin() + _size; }

private:
  const T *_data;
  size_t   _size;
  size_t   _off;
};
