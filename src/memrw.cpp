#include "memrw.hpp"


MemRW::~MemRW()
{

}

void
MemRW::reset(uint8_t       *data_,
             const size_t  size_)
{
  _data = data_;
  _size = size_;
  _idx  = 0;
}

bool
MemRW::eof() const
{
  return (_idx >= _size);
}

size_t
MemRW::tell() const
{
  return _idx;
}

void
MemRW::seek(const size_t idx_)
{
  _idx = idx_;
}

size_t
MemRW::read(uint8_t       *p_,
            const size_t  count_)
{
  size_t rv;

  rv = 0;
  for(size_t i = 0; i < count_ && !eof(); i++)
    {
      *p_++ = _data[_idx++];
      rv++;
    }

  return rv;
}

size_t
MemRW::write(uint8_t const * const p_,
             const size_t         count_)
{
  size_t rv;
  uint8_t const *p = p_;

  rv = 0;
  for(size_t i = 0; i < count_ && !eof(); i++)
    {
      _data[_idx++] = *p++;
      rv++;
    }

  return rv;
}
