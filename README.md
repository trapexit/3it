# 3it: 3DO Image Tool

An all purpose 3DO image conversion tool. Can convert to and from JPEG, PNG,
BMP, 3DO CEL, 3DO Banner, 3DO ANIM, 3DO IMAG, and NFS 3SH formats. Supports
coded and uncoded, packed and unpacked, linear and lrform CELs.


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
```


## TODO

* dump APPSCRN from ISO
* dump-chunks
* concat-chunks
* to IMAG
* to ANIM
* ability to write text chunks
* figure out NFS HSPT chunk


## Notes

I wrote this over the course of a few months on and off. I was playing with some
different ideas so the code is not entirely consistent. Will clean up as needed.


## Documentation

* https://3dodev.com/ext/3DO/Portfolio_2.5/OnLineDoc/DevDocs/ppgfldr/ggsfldr/gpgfldr/5gpg.html
* https://3dodev.com/documentation/file_formats/games/nfs


## External sites

* https://3dodev.com
* https://github.com/trapexit/3dt
* https://github.com/trapexit/3do-devkit
