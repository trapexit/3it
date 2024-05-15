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

#define FMT_HEADER_ONLY
#include "fmt/core.h"
#include "fmt/args.h"
#include "fmt/printf.h"

#include <exception>
#include <filesystem>

template<>
struct fmt::formatter<std::filesystem::path> : formatter<std::string>
{
  template <typename FormatContext>
  auto
  format(const std::filesystem::path &path_,
         FormatContext               &ctx_)
  {
    return formatter<std::string>::format(path_.string(),ctx_);
  }
};

namespace fmt
{
  static
  inline
  std::runtime_error
  vexception(fmt::string_view fmt_,
             fmt::format_args args_)
  {
    std::string s;

    s = fmt::vformat(fmt_,args_);

    return std::runtime_error(s);
  }

  template<typename Str, typename... Args>
  static
  inline
  std::runtime_error
  exception(Str const &fmt_,
            Args &&... args_)
  {
    return fmt::vexception(fmt_,
                           fmt::make_format_args(args_...));

  }
}
