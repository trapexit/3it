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

#include "byteswap.hpp"
#include "chunk.hpp"

#include "fmt.hpp"

#include <cctype>

const
uint8_t*
Chunk::base() const
{
  return _chunk.data();
}

uint32_t
Chunk::id(cspan<uint8_t> chunk_)
{
  const uint32_t *p;

  p = (const uint32_t*)chunk_.data();

  return ::byteswap_if_little_endian(p[0]);
}

uint32_t
Chunk::id() const
{
  return Chunk::id(_chunk);
}

std::string
Chunk::idstr(const uint32_t chunk_)
{
  cspan<uint8_t> chunk((const uint8_t*)&chunk_,sizeof(chunk_));

  return Chunk::idstr(chunk);
}

std::string
Chunk::idstr(cspan<uint8_t> chunk_)
{
  std::string s;

  s += chunk_[0];
  s += chunk_[1];
  s += chunk_[2];
  s += chunk_[3];

  return s;
}

std::string
Chunk::idstr() const
{
  return Chunk::idstr(_chunk);
}

uint32_t
Chunk::size(cspan<uint8_t> chunk_)
{
  const uint32_t *p;

  p = (const uint32_t*)chunk_.data();

  return byteswap_if_little_endian(p[1]);
}

uint32_t
Chunk::size() const
{
  return Chunk::size(_chunk);
}

const
uint8_t*
Chunk::data(cspan<uint8_t> chunk_)
{
  return &chunk_[sizeof(uint32_t) + sizeof(uint32_t)];
}

const
uint8_t*
Chunk::data() const
{
  return Chunk::data(_chunk);
}

uint32_t
Chunk::data_size(cspan<uint8_t> chunk_)
{
  return (Chunk::size(chunk_) - sizeof(uint32_t) - sizeof(uint32_t));
}

uint32_t
Chunk::data_size() const
{
  return Chunk::data_size(_chunk);
}

Chunk&
Chunk::operator=(cspan<uint8_t> data_)
{
  size_t size;

  validate(data_);

  size = Chunk::size(data_);
  _chunk = cspan<uint8_t>(data_.data(),size);

  return *this;
}

std::string
Chunk::validate(cspan<uint8_t> chunk_)
{
  uint32_t size;

  if(chunk_.size() < (sizeof(uint32_t) + sizeof(uint32_t)))
    return fmt::format("invalid chunk size - {} bytes",chunk_.size());

  if(!isprint(chunk_[0]) ||
     !isprint(chunk_[1]) ||
     !isprint(chunk_[2]) ||
     !isprint(chunk_[3]))
    return fmt::format("invalid chunk id - contains non-printable chars"
                       " - '{:c}{:c}{:c}{:c}'",
                       (isprint(chunk_[0]) ? chunk_[0] : '?'),
                       (isprint(chunk_[1]) ? chunk_[1] : '?'),
                       (isprint(chunk_[2]) ? chunk_[2] : '?'),
                       (isprint(chunk_[3]) ? chunk_[3] : '?'));

  size = Chunk::size(chunk_);
  if(chunk_.size() < size)
    return fmt::format("invalid chunk size - less than buffer size - {} < {}",chunk_.size(),size);
  if(size > (1024 * 1024 * 64))
    return fmt::format("invalid chunk size - {} > 64MB",size);

  return {};
}
