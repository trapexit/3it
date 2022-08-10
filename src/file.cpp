#include "file.hpp"

#include "byteswap.hpp"


File::File()
  : _file(nullptr),
    _big_endian(false)
{

}

void
File::open(const std::filesystem::path &filepath_,
           const char                  *mode_)
{
  _file = fopen(filepath_.string().c_str(),mode_);
  if((_file == NULL) || (ferror(_file) != 0))
    throw std::system_error(errno,std::system_category(),"failed to open "+filepath_.string());
}

void
File::close()
{
  if(_file)
    fclose(_file);
  _file = nullptr;
}

void
File::big_endian()
{
  _big_endian = true;
}

void
File::little_endian()
{
  _big_endian = false;
}

size_t
File::write(const void   *ptr_,
            const size_t  size_)
{
  return fwrite(ptr_,1,size_,_file);
}

size_t
File::read(void         *ptr_,
           const size_t  size_)
{
  return fread(ptr_,1,size_,_file);
}

size_t
File::write(uint8_t v_)
{
  return write(&v_,sizeof(v_));
}

size_t
File::write(uint16_t v_)
{
  if(_big_endian)
    v_ = byteswap_if_little_endian(v_);
  return write(&v_,sizeof(v_));
}

size_t
File::write(uint32_t v_)
{
  if(_big_endian)
    v_ = byteswap_if_little_endian(v_);
  return write(&v_,sizeof(v_));
}

size_t
File::write(cspan<uint8_t> data_)
{
  return write(data_.data(),data_.size());
}

int
File::seek(long offset_,
           int  whence_)
{
  return fseek(_file,offset_,whence_);
}

bool
File::eof()
{
  return feof(_file);
}

File::operator bool()
{
  if(_file == nullptr)
    return false;
  if(ferror(_file))
    return false;

  return true;
}
