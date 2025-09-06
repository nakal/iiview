
# iiview for Linux/Unix - by nakal

| Repository moved to: `https://codeberg.org/nakal/iiview` |
|---|

**iiview is an image viewer - mainly for X** - that allows
easy navigation through thumb views and full screen
view. The most notable feature is that it only has a
few dependencies, because it operates directly ontop of
the X API.

## Screenshots

### Default iiview thumb view

![iiview thumb view](https://lh4.googleusercontent.com/-SBk3VMOES9o/VIxf2Y3XOXI/AAAAAAAAGHo/lMfC2rUoM3A/w862-h767-no/iiview-thumbview.jpg)

### Full screen view

![iiview fullscreen view](https://lh3.googleusercontent.com/-yvELjirxeDU/VIxf1jAjcgI/AAAAAAAAGHg/adbFDgRh0Qk/w862-h767-no/iiview-fullscreen.jpg)

## For the impatient people

1) Type in: "make" or "make withfb" or "make withfbonly"
(depends on your system and your preferences)

make		- X-Window supportonly
		  This is default for all platforms.

make withfb     - X-Window and framebuffer support (read below)
		  This is for Linux framebuffer

make withfbonly - framebuffer support only
		  This is for people who don't want to have
		  a dependency with X on slim boxes

2) make install

Only for root user. This will install iiview in /usr/local/bin
Optionally, you can move the file ./bin/iiview to an
appropriate directory.


## Dependencies

### Supported systems

- Linux
- FreeBSD

Other systems and platforms might work, too.

### Required libraries

* libjpeg
* libncurses (only for Linux framebuffer support)
* libtiff
* libpng
* libz

## Problems

### If you cannot compile

For other distributions or non-default settings,
it's required to set the paths for include and
library files properly. See: "Makefile" and the
man pages about gcc/clang.
(You can try to "locate" the required includes
and libs.)

You will need to adjust some variables.
Please look inside the Makefile, there are
some hints how to do that.

### Usage

Pressing 'H' while running iiview will
give you some hints about the functions.

### Some words about FRAMEBUFFER

To use framebuffer-device, you should at least have
the kernel version 2.1.36 (try to get >2.2.0).
Enable the framebuffer-support in the configuration.
Make sure you use graphics displaying MORE THAN
256 colors, so that you don't need palette operations.
(256 colors are NOT SUPPORED)

See the directory /usr/src/linux/Documentation/fb
for further details about setting up the frambuffer
device.

Currently iiview uses the framebuffer device /dev/fb0
as default device. You can select other devices by
setting the FRAMEBUFFER environment variable.

If the device doesn't exist, create it according to
the Linux kernel documentation.

### Problems with FRAMEBUFFER?

Then you are forced to "make" only... :(

## Configuration file ~/.iiviewrc

After first start the file *.iiviewrc* will be created
in your home  directory.
Nothing special about it. It only saves the last
window coordinates from the X-session.

When something is wrong with the coordinates in
X, try to edit or delete ~/.iiviewrc.

# Have fun

I hope, everything works. If not, feel free to report it.
