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

#include "bytevec.hpp"
#include "chunkid.hpp"
#include "span.hpp"

#include <cstdint>
#include <string>
#include <vector>


class Chunk
{
public:
  static uint32_t id(cspan<uint8_t>);
  static std::string idstr(const uint32_t);
  static std::string idstr(cspan<uint8_t>);
  static uint32_t size(cspan<uint8_t>);
  static const uint8_t *data(cspan<uint8_t>);
  static uint32_t data_size(cspan<uint8_t>);
  static std::string validate(cspan<uint8_t>);

public:
  const uint8_t *base() const;
  uint32_t       id() const;
  std::string    idstr() const;
  uint32_t       size() const;
  const uint8_t *data() const;
  uint32_t       data_size() const;

public:
  Chunk& operator=(cspan<uint8_t>);

public:
  operator bool() const { return Chunk::validate(_chunk).empty(); }

private:
  cspan<uint8_t> _chunk;
};

typedef std::vector<Chunk> ChunkVec;
