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

size_t
VecRW::tell() const
{
  return _idx;
}

void
VecRW::seek(const size_t idx_)
{
  _idx = idx_;
  if(_idx >= _vec->size())
    _vec->resize(_idx);
}

size_t
VecRW::_r(void         *p_,
          const size_t  count_)
{
  size_t rv;
  uint8_t *p;

  rv = 0;
  p = (uint8_t*)p_;
  for(size_t i = 0; i < count_ && !eof(); i++)
    {
      *p++ = (*_vec)[_idx++];
      rv++;
    }

  return rv;
}

size_t
VecRW::_w(const void   *p_,
          const size_t  count_)
{
  std::size_t rv;
  std::uint8_t const *p = (std::uint8_t const*)p_;

  if((_idx + count_) > _vec->size())
    _vec->resize(_vec->size() + count_);

  rv = 0;
  for(size_t i = 0; i < count_; i++)
    {
      (*_vec)[_idx++] = *p++;
      rv++;
    }

  return rv;
}
