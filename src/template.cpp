/*
  ISC License

  Copyright (c) 2023, Antonio SJ Musumeci <trapexit@spawn.link>

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

#include "template.hpp"
#include "fmt.hpp"

std::string
resolve_template(std::filesystem::path const &input_filepath_,
                 std::filesystem::path const &output_filepath_,
                 std::string const           &ext_,
                 std::unordered_map<std::string,std::string> const &extra_)
{
  std::string rv;
  fmt::dynamic_format_arg_store<fmt::format_context> args;

  args.push_back(fmt::arg("filepath",input_filepath_));
  args.push_back(fmt::arg("dirpath",(input_filepath_.has_parent_path() ?
                                     input_filepath_.parent_path() : ".")));
  args.push_back(fmt::arg("filename",input_filepath_.stem()));
  args.push_back(fmt::arg("ext",input_filepath_.extension()));
   args.push_back(fmt::arg("dstext",ext_));

  for(auto &kv : extra_)
    args.push_back(fmt::arg(kv.first.c_str(),kv.second));

  rv = fmt::vformat(output_filepath_.string(),args);

  return rv;
}
