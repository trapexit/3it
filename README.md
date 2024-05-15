# 3it: 3DO Image Tool

An all purpose 3DO image conversion tool. Can convert to and from JPEG, PNG,
BMP, 3DO CEL, 3DO Banner, 3DO ANIM, 3DO IMAG, NFS 3SH, and NFS wwww formats.
Supports coded and uncoded, packed and unpacked, linear and lrform CELs.


## Usage

```
$ 3it --help
3it: 3DO Image Tool
Usage: 3it [OPTIONS] SUBCOMMAND

Options:
  -h,--help                   Print this help message and exit
    --help-all

Subcommands:
  version                     print 3it version and license
  docs                        print links to relevant documentation
  info                        prints info about the file
  list-chunks                 list 3DO file chunks
  to-cel                      convert image to 3DO CEL
  to-banner                   convert image to banner
  to-imag                     convert image to 3DO IMAG
  to-lrform                   convert image to raw LRFORM
  to-bmp                      convert image to BMP
  to-png                      convert image to PNG
  to-jpg                      convert image to JPG
  to-nfs-shpm                 convert image to Need for Speed SHPM

$ 3it to-cel --help
...
```

All subcommands have their own help and arguments. Use `--help` or
`--help-all` to see all available options.


## File Types

### LRFORM

It is common to use IMAG files as a background that is used to clear
the screen between screen updates by leveraging SPORT VRAM -> VRAM
copying. However, due to alignment requirements and needing to read
data off of a CDROM the `LoadImage()` function utilizes more resources
than needed. Especially given LoadImage were never expanded to support
anything but 320x240 @ 16bpp LRFORM images.

Having a raw LRFORM file allows the user to load it using
LoadFile and saves on resources. See [LoadImage's
code](https://github.com/trapexit/portfolio_os/blob/master/src/libs/lib3DO/DisplayUtils/LoadImage.c)
for more details.


```C
int memflags;
int filesize;
char *image;
char *filename = "image.lrform";

memflags = (MEMTYPE_TRACKSIZE|MEMTYPE_STARTPAGE|MEMTYPE_VRAM);
image    = LoadFile(filename,&filesize,memflags);
```

## Output Filename Templates

Any command with an `--output-path` option supports templating to make
scripting easier. It uses a currly brace notation such as
`{foo}`. Below is a list of supported values per subcommand.

All support:

* {filepath}: Full original input filepath.
* {dirpath}: Parent path of {filepath} if it has one. "." otherwise.
* {filename}: Just the filename without dirpath or extension.
* {origext}: The original filepath's extension. Includes prefixed '.'.
* {ext}: The standard extension for the target file.
* {_name}: If supported by the input the internal name for the
  image. Prefixed with "_". Otherwise "".
* {index}: The index of the image if the input had multiple images.
* {_index}: The index prefixed with "_" if input had multiple
  images. Otherwise "".
* {w}: Width of the output image.
* {h}: Height of the output image.

### to-cel

* {coded}: "coded" if coded else "uncoded".
* {packed}: "packed" if packed else "unpacked".
* {lrform}: "lrform" if lrform else "linear".
* {_lrform}: "_lrform" if lrform otherwise "".
* {bpp}: The CEL bits per pixel.
* {flags}: CCB flags in hex.
* {pixc}: CCB PIXC/PPMPC in hex.
* {rotation}: "0", "90", "180", or "270"


### to-banner

* {bpp}: Always 16.


### Others

No extras.


## TODO

* dump APPSCRN from ISO
* dump-chunks
* concat-chunks
* to ANIM
* ability to write text chunks
* figure out NFS HSPT chunk
* ability to write NFS wwww files? May deserve its own tool.
* Other game formats


## Documentation

* https://3dodev.com
* https://3dodev.com/documentation/file_formats
* https://3dodev.com/documentation/development/opera/pf25/ppgfldr/ggsfldr/gpgfldr/3gpg
* https://3dodev.com/documentation/development/opera/pf25/ppgfldr/ggsfldr/gpgfldr/5gpg
* https://3dodev.com/documentation/development/opera/pf25/ppgfldr/ggsfldr/gpgfldr/00gpg1


## Other Links

* 3DO Dev Repo: https://3dodev.com
* 3DO Disc Tool: https://github.com/trapexit/3dt
* 'Modern' 3DO DevKit: https://github.com/trapexit/3do-devkit
