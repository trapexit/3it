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
  version                     print 3it version
  docs                        print links to relevant documentation
  info                        prints info about the file
  list-chunks                 list 3DO file chunks
  to-cel                      convert image to CEL
  to-banner                   convert image to banner
  to-bmp                      convert image to BMP
  to-png                      convert image to PNG
  to-jpg                      convert image to JPG
  to-nfs-shpm                 convert image to Need for Speed SHPM

$ 3it to-cel --help
...
```

All subcommands have their own help and arguments. Use `--help` or
`--help-all` to see all available options.


## TODO

* dump APPSCRN from ISO
* dump-chunks
* concat-chunks
* to IMAG
* to ANIM
* ability to write text chunks
* figure out NFS HSPT chunk
* ability to write NFS wwww files? May deserve its own tool.
* Other game formats


## Notes

I wrote this over the course of a few months on and off. I was playing with some
different ideas so the code is not entirely consistent. Will clean up as needed.


## Documentation

* https://3dodev.com/documentation/development/opera/pf25/ppgfldr/ggsfldr/gpgfldr/5gpg
* https://3dodev.com/documentation/file_formats/media/container/3do
* https://3dodev.com/documentation/file_formats/media/image/bannerscreen
* https://3dodev.com/documentation/file_formats/games/nfs


## Other Links

* https://3dodev.com
* https://github.com/trapexit/3dt
* https://github.com/trapexit/3do-devkit
