#include "filerw.hpp"

#include <errno.h>


FileRW::FileRW()
  : _file(NULL)
{

}

FileRW::~FileRW()
{
  if(_file)
    fclose(_file);
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
  return open(filepath_,"w+");
}

int
FileRW::open_write_trunc(std::filesystem::path const &filepath_)
{
  return open_write_trunc(filepath_.string());
}

int
FileRW::error()
{
  return ferror(_file);
}

bool
FileRW::eof() const
{
  return feof(_file);
}

size_t
FileRW::tell() const
{
  return ftell(_file);
}

void
FileRW::seek(const size_t idx_)
{
  fseek(_file,idx_,SEEK_SET);
}

size_t
FileRW::read(uint8_t       *p_,
             const size_t  count_)
{
  size_t rv;

  rv = fread((void*)p_,1,count_,_file);

  return rv;
}

size_t
FileRW::write(uint8_t const * const p_,
              const size_t         count_)
{
  size_t rv;

  rv = fwrite((const void*)p_,1,count_,_file);

  return rv;
}
