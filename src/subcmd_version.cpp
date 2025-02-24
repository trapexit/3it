/*
  ISC License

  Copyright (c) 2025, Antonio SJ Musumeci <trapexit@spawn.link>

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

#include "fmt.hpp"

#include "version.hpp"

namespace SubCmd
{
  void
  version()
  {
    fmt::print("3it: 3DO Image Tool v{}.{}.{}\n\n"
               "https://github.com/trapexit/3it\n"
               "https://github.com/trapexit/support\n\n"
               "ISC License (ISC)\n\n"
               "Copyright 2025, Antonio SJ Musumeci <trapexit@spawn.link>\n\n"
               "Permission to use, copy, modify, and/or distribute this software for\n"
               "any purpose with or without fee is hereby granted, provided that the\n"
               "above copyright notice and this permission notice appear in all\n"
               "copies.\n\n"
               "THE SOFTWARE IS PROVIDED \"AS IS\" AND THE AUTHOR DISCLAIMS ALL\n"
               "WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED\n"
               "WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE\n"
               "AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL\n"
               "DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR\n"
               "PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER\n"
               "TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR\n"
               "PERFORMANCE OF THIS SOFTWARE.\n\n"
               ,
               MAJOR,
               MINOR,
               PATCH);
  }
}
