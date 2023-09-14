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
VecRW::read(uint8_t       *p_,
            const size_t  count_)
{
  size_t rv;

  rv = 0;
  for(size_t i = 0; i < count_ && !eof(); i++)
    {
      *p_++ = _vec->operator[](_idx++);
      rv++;
    }

  return rv;
}

size_t
VecRW::write(uint8_t const * const p_,
             const size_t         count_)
{
  std::size_t rv;
  std::uint8_t const *p = p_;

  if((_idx + count_) > _vec->size())
    _vec->resize(_vec->size() + count_);

  rv = 0;
  for(size_t i = 0; i < count_; i++)
    {
      _vec->operator[](_idx++) = *p++;
      rv++;
    }

  return rv;
}
