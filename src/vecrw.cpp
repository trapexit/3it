#include "vecrw.hpp"


VecRW::~VecRW()
{

}

void
VecRW::reset(std::vector<uint8_t> *vec_)
{
  _vec = vec_;
  _idx = 0;
}

bool
VecRW::eof() const
{
  return (_idx >= _vec->size());
}

bool
VecRW::error() const
{
  return false;
}

uint64_t
VecRW::tell() const
{
  return _idx;
}

void
VecRW::seek(const uint64_t idx_)
{
  _idx = idx_;
  if(_idx >= _vec->size())
    _vec->resize(_idx);
}

uint64_t
VecRW::_r(void         *p_,
          const uint64_t  count_)
{
  uint64_t rv;
  uint8_t *p;

  rv = 0;
  p = (uint8_t*)p_;
  for(uint64_t i = 0; i < count_ && !eof(); i++)
    {
      *p++ = (*_vec)[_idx++];
      rv++;
    }

  return rv;
}

uint64_t
VecRW::_w(const void   *p_,
          const uint64_t  count_)
{
  std::uint64_t rv;
  std::uint8_t const *p = (std::uint8_t const*)p_;

  if((_idx + count_) > _vec->size())
    _vec->resize(_vec->size() + count_);

  rv = 0;
  for(uint64_t i = 0; i < count_; i++)
    {
      (*_vec)[_idx++] = *p++;
      rv++;
    }

  return rv;
}
