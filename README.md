# amiga-demo-1

## Introduction

Amiga demo effects implemented in an OS friendly way. No direct
hardware access (besides sprite dma disable) is used. AmigaOS
is not deactivated and kickstart APIs are used to write on screen.
Implements the following effects:

* Shows stars, a planet and text scrolls in from right to left.
  Characters are loaded from a custom font painted by me ;)
* When text scroller has finished, the screen fades to white
* When the screen is completely white, it fades back and displays
  a logo

## Build

Can be 

* crosscompiled with makefile and [GCC](http://aminet.net/package/dev/gcc/m68k-amigaos-gcc)
* on a real machine with SAS-C and smake

## History

Originally, I started around 2015 to code for my favorite childhood platform.
On youtube, the tutorials by [Photon of Scoopex](https://www.youtube.com/channel/UC1lfCoAuwbQ22H-KoImEygg)
helped alot to understand the platform 

...

When I started this project, I just had finished 
[Amiga Starlight Frameowkr](https://github.com/b3lial/amiga-starlight-framework). 

## Architecture

...