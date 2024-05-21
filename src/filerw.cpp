#include "filerw.hpp"

#include <cstdint>
#include <errno.h>


FileRW::FileRW()
  : _file(NULL)
{

}

FileRW::~FileRW()
{
  close();
}

int
FileRW::open(char const *filepath_,
             char const *mode_)
{
  _file = fopen(filepath_,mode_);

  return ((_file == NULL) ? -errno : 0);
}

int
FileRW::open(std::string const &filepath_,
             std::string const &mode_)
{
  return open(filepath_.c_str(),mode_.c_str());
}

int
FileRW::open_write_trunc(std::string const &filepath_)
{
  return open(filepath_,"w+b");
}

int
FileRW::open_write_trunc(std::filesystem::path const &filepath_)
{
  return open_write_trunc(filepath_.string());
}

int
FileRW::close()
{
  int rv;

  if(_file == NULL)
    return 0;

  rv = fclose(_file);

  _file = NULL;

  return rv;
}

bool
FileRW::eof() const
{
  if(_file)
    return feof(_file);
  return true;
}

bool
FileRW::error() const
{
  if(_file)
    return ferror(_file);
  return true;
}

uint64_t
FileRW::tell() const
{
  return ftell(_file);
}

void
FileRW::seek(const uint64_t idx_)
{
  fseek(_file,idx_,SEEK_SET);
}

uint64_t
FileRW::_r(void         *p_,
           const uint64_t  count_)
{
  uint64_t rv;

  rv = fread((void*)p_,1,count_,_file);

  return rv;
}

uint64_t
FileRW::_w(const void   *p_,
           const uint64_t  count_)
{
  uint64_t rv;

  rv = fwrite((const void*)p_,1,count_,_file);

  return rv;
}
