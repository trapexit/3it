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

#include "memrw.hpp"
#include "filerw.hpp"
#include "version.hpp"

#include "subcmd.hpp"

#include "CLI11.hpp"
#include "fmt.hpp"

#include <filesystem>

#include <locale>

namespace fs = std::filesystem;

static
std::string
color2rgb_transform(std::string &s_)
{
  if(s_ == "black")
    s_ = "0x000000FF";
  else if(s_ == "white")
    s_ = "0xFFFFFFFF";
  else if(s_ == "magenta")
    s_ = "0xFF00FFFF";
  else if(s_ == "cyan")
    s_ = "0x00FFFFFF";
  else if(s_ == "red")
    s_ = "0xFF0000FF";
  else if(s_ == "green")
    s_ = "0x00FF00FF";
  else if(s_ == "blue")
    s_ = "0x0000FFFF";

  return {};
}

static
void
generate_version_argparser(CLI::App &app_)
{
  CLI::App *subcmd;

  subcmd = app_.add_subcommand("version","print 3it version");

  subcmd->callback(std::bind(SubCmd::version));
}

static
void
generate_docs_argparser(CLI::App &app_)
{
  CLI::App *subcmd;

  subcmd = app_.add_subcommand("docs","print links to relevant documentation");

  subcmd->callback(std::bind(SubCmd::docs));
}

static
void
generate_info_argparser(CLI::App      &app_,
                        Options::Info &options_)
{
  CLI::App *subcmd;

  subcmd = app_.add_subcommand("info","prints info about the file");
  subcmd->add_option("filepaths",options_.filepaths)
    ->description("path to file")
    ->type_name("PATH")
    ->check(CLI::ExistingFile)
    ->required();

  subcmd->callback(std::bind(SubCmd::info,std::cref(options_)));
}

static
void
generate_list_chunks(CLI::App            &app_,
                     Options::ListChunks &opts_)
{
  CLI::App *subcmd;

  subcmd = app_.add_subcommand("list-chunks","list 3DO file chunks");
  subcmd->add_option("filepaths",opts_.filepaths)
    ->description("path to images")
    ->type_name("PATH")
    ->check(CLI::ExistingFile)
    ->required();

  subcmd->callback(std::bind(SubCmd::list_chunks,std::cref(opts_)));
}

static
void
generate_dump_packed_instructions(CLI::App            &app_,
                                  Options::DumpPacked &opts_)
{
  CLI::App *subcmd;

  subcmd = app_.add_subcommand("dump-packed-instructions",
                               "print out a packed CEL's instruction list");
  subcmd->alias("dpi");
  subcmd->add_option("filepath",opts_.filepath)
    ->description("path to packed CEL image")
    ->type_name("PATH")
    ->check(CLI::ExistingFile)
    ->required();

  subcmd->callback(std::bind(SubCmd::dump_packed_instructions,
                             std::cref(opts_)));
}

#define ADD_FLAG(NAME)                                          \
  subcmd_->add_option("--ccb-"#NAME,flags_.NAME)                \
  ->description("Set CCB flag "#NAME)                           \
  ->default_str("default")                                      \
  ->transform(CLI::CheckedTransformer(map,CLI::ignore_case))    \
  ->take_last()

static
void
generate_ccb_flag_argparser(CLI::App          *subcmd_,
                            Options::CCBFlags &flags_)
{
  const std::map<std::string,Options::Flag> map =
    {{"default",Options::Flag::DEFAULT},
     {"set",Options::Flag::SET},
     {"unset",Options::Flag::UNSET}};

  ADD_FLAG(skip);
  ADD_FLAG(last);
  ADD_FLAG(npabs);
  ADD_FLAG(spabs);
  ADD_FLAG(ppabs);
  ADD_FLAG(ldsize);
  ADD_FLAG(ldprs);
  ADD_FLAG(ldplut);
  ADD_FLAG(ccbpre);
  ADD_FLAG(yoxy);
  ADD_FLAG(acsc);
  ADD_FLAG(alsc);
  ADD_FLAG(acw);
  ADD_FLAG(accw);
  ADD_FLAG(twd);
  ADD_FLAG(lce);
  ADD_FLAG(ace);
  ADD_FLAG(maria);
  ADD_FLAG(pxor);
  ADD_FLAG(useav);
  ADD_FLAG(packed);
  ADD_FLAG(plutpos);
  ADD_FLAG(bgnd);
  ADD_FLAG(noblk);
}

#undef ADD_FLAG

#define ADD_FLAG(NAME)                                          \
  subcmd_->add_option("--pre0-"#NAME,flags_.NAME)               \
  ->description("Set PRE0 flag "#NAME)                          \
  ->default_str("default")                                      \
  ->transform(CLI::CheckedTransformer(map,CLI::ignore_case))    \
  ->take_last()

static
void
generate_pre0_flag_argparser(CLI::App           *subcmd_,
                             Options::Pre0Flags &flags_)
{
  const std::map<std::string,Options::Flag> map =
    {{"default",Options::Flag::DEFAULT},
     {"set",Options::Flag::SET},
     {"unset",Options::Flag::UNSET}};

  ADD_FLAG(literal);
  ADD_FLAG(bgnd);
  ADD_FLAG(uncoded);
  ADD_FLAG(rep8);
}

#undef ADD_FLAG

static
void
generate_to_cel_argparser(CLI::App       &app_,
                          Options::ToCEL &options_)
{
  CLI::App *subcmd;
  std::string default_output_path;

  default_output_path = "{filepath}{_index}{_name}{ext}";

  subcmd = app_.add_subcommand("to-cel","convert image to CEL");
  subcmd->add_option("filepaths",options_.filepaths)
    ->description("Path to image or directory")
    ->type_name("PATH")
    ->check(CLI::ExistingPath)
    ->required();
  subcmd->add_option("-o,--output-path",options_.output_path)
    ->description("Path to output file")
    ->type_name("PATH")
    ->default_val(default_output_path)
    ->default_str(default_output_path)
    ->take_last();
  subcmd->add_option("-b,--bpp",options_.bpp)
    ->description("Bits per pixel")
    ->type_name("BPP")
    ->default_val(16)
    ->check(CLI::IsMember({1,2,4,6,8,16}))
    ->take_last();
  subcmd->add_option("--coded",options_.coded)
    ->description("Store coded CEL")
    ->default_val(false)
    ->default_str("false")
    ->take_last();
  subcmd->add_option("--lrform",options_.lrform)
    ->description("Store CEL in LRFORM")
    ->default_val(false)
    ->default_str("false")
    ->take_last();
  subcmd->add_option("--packed",options_.packed)
    ->description("Pack pixel data")
    ->default_val(false)
    ->default_str("false")
    ->take_last();
  subcmd->add_option("--transparent",options_.transparent)
    ->description("Set packed pixel transparent color")
    ->type_name("HEX_RGBA32")
    ->option_text("COLOR:{black,white,red,green,blue,magenta,cyan,0xRRGGBBAA} [magenta]")
    ->transform(CLI::Validator(color2rgb_transform,""))
    ->default_val("magenta")
    ->take_last();
  subcmd->add_option("-i,--ignore-target-ext",options_.ignore_target_ext)
    ->description("Ignore files with target extension")
    ->default_val(false)
    ->default_str("false")
    ->take_last();
  subcmd->add_option("--external-palette",options_.external_palette)
    ->description("Use a different CEL file's PLUT instead of building a unique one")
    ->type_name("PATH")
    ->check(CLI::ExistingFile)
    ->take_last();
  subcmd->add_option("--write-plut",options_.write_plut)
    ->description("Write PLUT to 3DO CEL file")
    ->default_val(true)
    ->default_str("true")
    ->take_last();
  subcmd->add_option("--rotation",options_.rotation)
    ->description("Set the degrees rotation")
    ->default_val(0)
    ->check(CLI::IsMember({0,90,180,270}))
    ->take_last();
  subcmd->add_flag("--generate-all",options_.generate_all)
    ->description("Generate every valid permutation of CEL type")
    ->default_val(false)
    ->default_str("false")
    ->excludes("--coded")
    ->excludes("--packed")
    ->excludes("--lrform")
    ->excludes("--bpp")
    ->excludes("--rotation")
    ->take_last();
  subcmd->add_option("--find-smallest",options_.find_smallest)
    ->description("Find the smallest CEL format")
    ->default_val("")
    ->check(CLI::IsMember({"regular","rotation"}))
    ->excludes("--generate-all")
    ->excludes("--coded")
    ->excludes("--packed")
    ->excludes("--lrform")
    ->excludes("--bpp")
    ->excludes("--rotation")
    ->take_last();
  generate_ccb_flag_argparser(subcmd,options_.ccb_flags);
  generate_pre0_flag_argparser(subcmd,options_.pre0_flags);
  subcmd->footer("Output Path Template Values:\n"
                 "  {filepath}: input filepath\n"
                 "  {dirpath}: base path of filepath\n"
                 "  {filename}: just the input filename without extension\n"
                 "  {origext}: input file extension (with .)\n"
                 "  {ext}: '.cel'\n"
                 "  {coded}: 'coded' or 'uncoded'\n"
                 "  {packed}: 'packed' or 'unpacked'\n"
                 "  {lrform}: 'lrform' or 'linear'\n"
                 "  {_lrform}: '_lrform' or ''\n"
                 "  {bpp}: '1', '2', '4', '6', '8', or '16'\n"
                 "  {w}: image width\n"
                 "  {h}: image height\n"
                 "  {flags}: hex of CCB Flags\n"
                 "  {pixc}: hex of CCB PIXC\n"
                 "  {rotation}: rotation of the bitmap relative to input in degrees\n"
                 "  {_name}: internal name of the image prefixed with '_' if supported, else empty\n"
                 "  {index}: index of image in input if contained multiple\n"
                 "  {_index}: index of image in input list if > 1 prepended with '_', else empty\n"
                 );


  subcmd->callback(std::bind(SubCmd::to_cel,
                             std::cref(options_)));
}

static
void
generate_to_banner_argparser(CLI::App          &app_,
                             Options::ToBanner &options_)
{
  CLI::App *subcmd;
  std::string default_output_path;

  default_output_path = "{filepath}{_index}{_name}{ext}";

  subcmd = app_.add_subcommand("to-banner","convert image to banner");
  subcmd->add_option("filepaths",options_.filepaths)
    ->description("Path to image or directory")
    ->type_name("PATH")
    ->check(CLI::ExistingPath)
    ->required();
  subcmd->add_option("-o,--output-path",options_.output_path)
    ->description("Path to output file")
    ->type_name("PATH")
    ->default_val(default_output_path)
    ->default_str(default_output_path)
    ->take_last();
  subcmd->add_option("-i,--ignore-target-ext",options_.ignore_target_ext)
    ->description("Ignore files with target extension")
    ->default_val(false)
    ->default_str("false")
    ->take_last();
  subcmd->footer("Output Path Template Values:\n"
                 "  {filepath}: input filepath\n"
                 "  {dirpath}: base path of filepath\n"
                 "  {filename}: just the input filename without extension\n"
                 "  {origext}: input file extension (with .)\n"
                 "  {ext}: '.banner'\n"
                 "  {bpp}: '16'\n"
                 "  {w}: image width\n"
                 "  {h}: image height\n"
                 "  {_name}: internal name of the image prefixed with '_' if supported, else empty\n"
                 "  {index}: index of image in input if contained multiple\n"
                 "  {_index}: index of image in input list if > 1 prepended with '_', else empty\n"
                 );

  subcmd->callback(std::bind(SubCmd::to_banner,
                             std::cref(options_)));
}

static
void
generate_to_imag_argparser(CLI::App        &app_,
                           Options::ToIMAG &options_)
{
  CLI::App *subcmd;
  std::string default_output_path;

  default_output_path = "{filepath}{_index}{_name}{ext}";

  subcmd = app_.add_subcommand("to-imag","convert image to IMAG");
  subcmd->add_option("filepaths",options_.filepaths)
    ->description("Path to image or directory")
    ->type_name("PATH")
    ->check(CLI::ExistingPath)
    ->required();
  subcmd->add_option("-o,--output-path",options_.output_path)
    ->description("Path to output file")
    ->type_name("PATH")
    ->default_val(default_output_path)
    ->default_str(default_output_path)
    ->take_last();
  subcmd->add_option("-i,--ignore-target-ext",options_.ignore_target_ext)
    ->description("Ignore files with target extension")
    ->default_val(false)
    ->default_str("false")
    ->take_last();
  subcmd->footer("Output Path Template Values:\n"
                 "  {filepath}: input filepath\n"
                 "  {dirpath}: base path of filepath\n"
                 "  {filename}: just the input filename without extension\n"
                 "  {origext}: input file extension (with .)\n"
                 "  {ext}: '.imag'\n"
                 "  {w}: image width\n"
                 "  {h}: image height\n"
                 "  {_name}: internal name of the image prefixed with '_' if supported, else empty\n"
                 "  {index}: index of image in input if contained multiple\n"
                 "  {_index}: index of image in input list if > 1 prepended with '_', else empty\n"
                 );

  subcmd->callback(std::bind(SubCmd::to_imag,
                             std::cref(options_)));
}

static
void
generate_to_lrform_argparser(CLI::App          &app_,
                             Options::ToLRFORM &options_)
{
  CLI::App *subcmd;
  std::string default_output_path;

  default_output_path = "{filepath}{_index}{_name}{ext}";

  subcmd = app_.add_subcommand("to-lrform","convert image to raw LRFORM");
  subcmd->add_option("filepaths",options_.filepaths)
    ->description("Path to image or directory")
    ->type_name("PATH")
    ->check(CLI::ExistingPath)
    ->required();
  subcmd->add_option("-o,--output-path",options_.output_path)
    ->description("Path to output file")
    ->type_name("PATH")
    ->default_val(default_output_path)
    ->default_str(default_output_path)
    ->take_last();
  subcmd->add_option("-i,--ignore-target-ext",options_.ignore_target_ext)
    ->description("Ignore files with target extension")
    ->default_val(false)
    ->default_str("false")
    ->take_last();
  subcmd->footer("Output Path Template Values:\n"
                 "  {filepath}: input filepath\n"
                 "  {dirpath}: base path of filepath\n"
                 "  {filename}: just the input filename without extension\n"
                 "  {origext}: input file extension (with .)\n"
                 "  {ext}: '.imag'\n"
                 "  {w}: image width\n"
                 "  {h}: image height\n"
                 "  {_name}: internal name of the image prefixed with '_' if supported, else empty\n"
                 "  {index}: index of image in input if contained multiple\n"
                 "  {_index}: index of image in input list if > 1 prepended with '_', else empty\n"
                 );

  subcmd->callback(std::bind(SubCmd::to_lrform,
                             std::cref(options_)));
}

static
void
generate_to_bmp_argparser(CLI::App         &app_,
                          Options::ToImage &options_)
{
  CLI::App *subcmd;
  std::string default_output_path;

  default_output_path = "{filepath}{_index}{_name}{ext}";

  subcmd = app_.add_subcommand("to-bmp","convert image to BMP");
  subcmd->add_option("filepaths",options_.filepaths)
    ->description("Path to image or directory")
    ->type_name("PATH")
    ->check(CLI::ExistingPath)
    ->required();
  subcmd->add_option("-o,--output-path",options_.output_path)
    ->description("Path to output file")
    ->type_name("PATH")
    ->default_val(default_output_path)
    ->default_str(default_output_path)
    ->take_last();
  subcmd->add_option("-i,--ignore-target-ext",options_.ignore_target_ext)
    ->description("Ignore files with target extension")
    ->default_val(false)
    ->default_str("false")
    ->take_last();
  subcmd->footer("Output Path Template Values:\n"
                 "  {filepath}: input filepath\n"
                 "  {dirpath}: base path of filepath\n"
                 "  {filename}: just the input filename without extension\n"
                 "  {origext}: input file extension (with .)\n"
                 "  {ext}: '.bmp'\n"
                 "  {w}: image width\n"
                 "  {h}: image height\n"
                 "  {_name}: internal name of the image prefixed with '_' if supported, else empty\n"
                 "  {index}: index of image in input if contained multiple\n"
                 "  {_index}: index of image in input list if > 1 prepended with '_', else empty\n"
                 );

  subcmd->callback(std::bind(SubCmd::to_stb_image,
                             std::cref(options_),
                             "bmp"));
}

static
void
generate_to_png_argparser(CLI::App         &app_,
                          Options::ToImage &options_)
{
  CLI::App *subcmd;
  std::string default_output_path;

  default_output_path = "{filepath}{_index}{_name}{ext}";

  subcmd = app_.add_subcommand("to-png","convert image to PNG");
  subcmd->add_option("filepaths",options_.filepaths)
    ->description("Path to image or directory")
    ->type_name("PATH")
    ->check(CLI::ExistingPath)
    ->required();
  subcmd->add_option("-o,--output-path",options_.output_path)
    ->description("Path to output file")
    ->type_name("PATH")
    ->default_val(default_output_path)
    ->default_str(default_output_path)
    ->take_last();
  subcmd->add_option("-i,--ignore-target-ext",options_.ignore_target_ext)
    ->description("Ignore files with target extension")
    ->default_val(false)
    ->default_str("false")
    ->take_last();
  subcmd->footer("Output Path Template Values:\n"
                 "  {filepath}: input filepath\n"
                 "  {dirpath}: base path of filepath\n"
                 "  {filename}: just the input filename without extension\n"
                 "  {origext}: input file extension (with .)\n"
                 "  {ext}: '.png'\n"
                 "  {w}: image width\n"
                 "  {h}: image height\n"
                 "  {_name}: internal name of the image prefixed with '_' if supported, else empty\n"
                 "  {index}: index of image in input if contained multiple\n"
                 "  {_index}: index of image in input list if > 1 prepended with '_', else empty\n"
                 );

  subcmd->callback(std::bind(SubCmd::to_stb_image,
                             std::cref(options_),
                             "png"));
}

static
void
generate_to_jpg_argparser(CLI::App         &app_,
                          Options::ToImage &options_)
{
  CLI::App *subcmd;
  std::string default_output_path;

  default_output_path = "{filepath}{_index}{_name}{ext}";

  subcmd = app_.add_subcommand("to-jpg","convert image to JPG");
  subcmd->add_option("filepaths",options_.filepaths)
    ->description("Path to image or directory")
    ->type_name("PATH")
    ->check(CLI::ExistingPath)
    ->required();
  subcmd->add_option("-o,--output-path",options_.output_path)
    ->description("Path to output file")
    ->type_name("PATH")
    ->default_val(default_output_path)
    ->default_str(default_output_path)
    ->take_last();
  subcmd->add_option("-i,--ignore-target-ext",options_.ignore_target_ext)
    ->description("Ignore files with target extension")
    ->default_val(false)
    ->default_str("false")
    ->take_last();
  subcmd->footer("Output Path Template Values:\n"
                 "  {filepath}: input filepath\n"
                 "  {dirpath}: base path of filepath\n"
                 "  {filename}: just the input filename without extension\n"
                 "  {origext}: input file extension (with .)\n"
                 "  {ext}: '.jpg'\n"
                 "  {w}: image width\n"
                 "  {h}: image height\n"
                 "  {_name}: internal name of the image prefixed with '_' if supported, else empty\n"
                 "  {index}: index of image in input if contained multiple\n"
                 "  {_index}: index of image in input list if > 1 prepended with '_', else empty\n"
                 );

  subcmd->callback(std::bind(SubCmd::to_stb_image,
                             std::cref(options_),
                             "jpg"));
}

static
void
generate_to_nfs_shpm(CLI::App           &app_,
                     Options::ToNFSSHPM &options_)
{
  CLI::App *subcmd;

  subcmd = app_.add_subcommand("to-nfs-shpm","convert image to NFS SHPM");
  subcmd->add_option("filepath",options_.filepaths)
    ->description("Path to source image")
    ->type_name("PATH")
    ->check(CLI::ExistingFile)
    ->required();
  subcmd->add_option("-o,--output-path",options_.output_path)
    ->description("Path to output file")
    ->type_name("PATH");
  subcmd->add_option("--packed",options_.packed)
    ->description("Pack pixel data")
    ->default_val(false)
    ->default_str("false")
    ->take_last();
  subcmd->add_option("--transparent",options_.transparent)
    ->description("Set packed pixel transparent color")
    ->type_name("HEX_RGBA32")
    ->option_text("COLOR:{black,white,red,green,blue,magenta,cyan,0xRRGGBBAA} [magenta]")
    ->transform(CLI::Validator(color2rgb_transform,""))
    ->default_val("magenta")
    ->take_last();

  subcmd->callback(std::bind(SubCmd::to_nfs_shpm,
                             std::cref(options_)));
}

static
void
generate_argparser(CLI::App &app_,
                   Options  &options_)
{
  app_.set_help_all_flag("--help-all",
                         "Print help all help messages and exit");
  app_.require_subcommand();

  generate_info_argparser(app_,options_.info);
  generate_to_cel_argparser(app_,options_.to_cel);
  generate_to_banner_argparser(app_,options_.to_banner);
  generate_to_imag_argparser(app_,options_.to_imag);
  generate_to_lrform_argparser(app_,options_.to_lrform);
  generate_to_nfs_shpm(app_,options_.to_nfs_shpm);
  generate_to_bmp_argparser(app_,options_.to_image);
  generate_to_png_argparser(app_,options_.to_image);
  generate_to_jpg_argparser(app_,options_.to_image);
  generate_list_chunks(app_,options_.list_chunks);
  generate_dump_packed_instructions(app_,options_.dump_packed);
  generate_version_argparser(app_);
  generate_docs_argparser(app_);
}

static
void
set_locale()
{
  try
    {
      std::locale::global(std::locale(""));
    }
  catch(const std::runtime_error &e)
    {
      std::locale::global(std::locale("C"));
    }
}

int
main(int    argc_,
     char **argv_)
{
  CLI::App app;  
  Options options;

  app.description(fmt::format("3it: 3DO Image Tool v{}.{}.{}",
                              MAJOR,MINOR,PATCH));
  
  set_locale();

  generate_argparser(app,options);

  try
    {
      app.parse(argc_,argv_);
    }
  catch(const CLI::ParseError &e_)
    {
      return app.exit(e_);
    }
  catch(const std::system_error &e_)
    {
      fmt::print("{} ({})\n",e_.what(),e_.code().message());
    }
  catch(const std::runtime_error &e_)
    {
      fmt::print("{}\n",e_.what());
    }

  return 0;
}
