/*
  ISC License

  Copyright (c) 2022, Antonio SJ Musumeci <trapexit@spawn.link>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#pragma once

#include "chunk.hpp"

#include <iterator>

class ChunkIter : public std::iterator<std::forward_iterator_tag,Chunk>
{
public:
  ChunkIter(cspan<uint8_t> data_)
    : _pos(data_.data()),
      _end(data_.data() + data_.size())
  {
  }

  ChunkIter(const ChunkIter &iter_)
    : _pos(iter_._pos),
      _end(iter_._end)
  {
  }

public:
  ChunkIter&
  operator++()
  {
    if(_pos < _end)
      {
        _pos += _chunk.size();
        _chunk = cspan<uint8_t>(_pos,_end);
      }
    else
      {
        _pos = _end = nullptr;
        _chunk = cspan<uint8_t>();
      }

    return *this;
  }

  ChunkIter
  operator++(int)
  {
    ChunkIter tmp(*this);

    operator++();

    return tmp;
  }

  bool
  operator==(const ChunkIter &rhs_) const
  {
    return (_chunk == rhs_._chunk);
  }

  bool
  operator!=(const ChunkIter &rhs_) const
  {
    return (_chunk != rhs_._chunk);
  }

  Chunk&
  operator*()
  {
    return _chunk;
  }

  static
  ChunkIter
  end()
  {
    return ChunkIter(cspan<uint8_t>());
  }

private:
  const uint8_t *_pos;
  const uint8_t *_end;
  Chunk          _chunk;
};
