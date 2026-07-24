# 3it: 3DO Image Tool

An all purpose 3DO image conversion tool. Can convert to and from JPEG, PNG,
BMP, 3DO CEL, 3DO Banner, 3DO ANIM, 3DO IMAG, NFS 3SH, and NFS wwww formats.
Supports coded and uncoded, packed and unpacked, linear and lrform CELs.

## Build

```bash
make
make NDEBUG=1
make SANITIZE=1
make clean
make release
```

Run `make help` for installation, cleanup, and direct Zig
cross-compilation targets.

## Usage

```
$ 3it --help
3it: 3DO Image Tool
Usage: 3it [OPTIONS] SUBCOMMAND

Options:
  -h,--help                   Print this help message and exit
    --help-all

Subcommands:
  info                        prints info about the file
  to-cel                      convert image to CEL
  to-banner                   convert image to banner
  to-imag                     convert image to IMAG
  to-lrform                   convert image to raw LRFORM
  to-nfs-shpm                 convert image to NFS SHPM
  to-bmp                      convert image to BMP
  to-png                      convert image to PNG
  to-jpg                      convert image to JPG
  list-chunks                 list 3DO file chunks
  dump-packed-instructions, dpi
                              print out a packed CEL's instruction list
  version                     print 3it version
  docs                        print links to relevant documentation

$ 3it to-cel --help
...
```

All subcommands have their own help and arguments. Use `--help` or
`--help-all` to see all available options.


## Notes

* All images are first converted to RGBA8888 before converting to the
  target format.
* No dithering is done by 3it when reducing bit depth.
* IMAG decoding applies embedded single-image and per-scanline VDL
  palettes. Other image types continue to assume the fixed display CLUT.
* When converting to coded (paletted) formats the number of colors
  will be checked. The transparent color is not included.
* The 3DO CEL renderer has many features. A good number of them are
  rarely used. As such 3it does not support all permutations of
  options when converting to or from supported formats. If an image
  looks wrong please file a
  [ticket](https://github.com/trapexit/3it/issues) and include as much
  information as you can including program arguments and the original
  source file.


## File Types

### IMAG

`to-imag` supports seven encodings selected with `--mode`:

* `fixed` writes the traditional 16-bit LRFORM image using the fixed
  display CLUT. This remains the default.
* `v480` writes a spatial 320x480 16-bit LRFORM image using the fixed display
  CLUT. All 480 source rows remain distinct. The IMAG does not embed a VDL and
  requires an external `VDL_480RES` display path; it is not compatible with
  the SDK's ordinary `LoadImage()` display path.
* `vdl` writes 16-bit LRFORM data with one custom VDL palette for the
  complete image.
* `xvdl` writes 16-bit LRFORM data with one custom VDL palette per
  logical scanline.
* `v480-vdl` is the 320x480 spatial form of `vdl`. Its VDL record selects
  480-resolution DMA and disables vertical interpolation.
* `v480-xvdl` is the 320x480 spatial form of `xvdl`, with one custom palette
  per spatial row and the same 480-resolution VDL controls.
* `z24` writes two 15-bit samples per logical pixel in the 32-bit paired-line
  format used by the 24-bit slideshow example. The IMAG does not embed a VDL:
  the slideshow's external 480/576-line VDL places the samples in the two
  display fields and enables vertical interpolation. The displayed component
  is the truncated average of the two programmed CLUT values, which reconstructs
  the logical 8-bit value exactly.

Custom VDL modes accept `--palette legacy` (the default SDK-compatible
box-population-length selection) or `--palette modern` (a deterministic
error-reducing refinement of the legacy palette).

Examples:

```
3it to-imag source.png --mode vdl --palette legacy -o source_1vdl.imag
3it to-imag source.png --mode xvdl --palette modern -o source_xvdl.imag
3it to-imag source_320x480.png --mode v480 -o source_480.imag
3it to-imag source_320x480.png --mode v480-vdl -o source_480_vdl.imag
3it to-imag source_320x480.png --mode v480-xvdl -o source_480_xvdl.imag
3it to-png source_480.imag -o source_320x480.png
3it to-imag source.png --mode z24 -o source_z24.imag
```

The 16-bit LRFORM modes require an even image height; all three `v480` modes
additionally require an exact 320x480 input. They store spatial vertical detail
rather than the paired color samples used by `z24`. The application must place
the image in the field buffers expected by its `VDL_480RES` setup. The custom
VDL records are included for palette/control data, but the application still
has to install and patch its 480-line display path; the SDK's ordinary
`LoadImage()` path does not do this. `z24` is intended for static displays using
the matching slideshow VDL; ordinary cels do not render normally into that
double-height framebuffer.

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
* ability to write NFS wwww files
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
