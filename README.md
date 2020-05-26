# jhpyle/mTCP

This is an unofficial fork of [mTCP] by Michael B. Brutman.

This fork makes the Telnet application in [mTCP] more usable when
connecting to Linux machines.  It adds:

* Extended keyboard support
* Mouse and clipboard support
* Translation of incoming Unicode characters
* [Sixel graphics]

The recompiled binary is available as the [TELNET.EXE] file, which is
in the [APPS/TELNET] directory.  It is based on Mr. Brutman's
`mTCP-src_2020-03-07.zip`.

## The changes

The purpose of this repository is to expand the capabilities of [mTCP]
Telnet's `ansi` terminal emulator.  Where a feature was not supported
by (what appears to be) the `ansi` standard, ideas were borrowed from
the `xterm` standard.  (The exception is the Ctrl-backspace keyboard
command, for which a custom escape sequence was used.)

### Keyboard

The following keys are transmitted:

* Function keys F1-F12;
* Insert, Delete, Home, End, Page Up, Page Down;
* Cursor keys;
* Shift, Ctrl, Ctrl-shift, Alt, and Alt-Shift modifications of the above;
* Tab, Ctrl-Tab, Shift-Tab;
* Ctrl-Backspace;
* Ctrl-Space;
* Ctrl-PrtScr.

In the official [mTCP], Page Up and Page Down scroll are not passed
through but are used for scrolling through the output history.  In
this version, that function is moved to the Alt-Page Up and Alt-Page
Down keys (or Ctrl-Page Up and Ctrl-Page Dn with [TEL8088.EXE]), and
Page Up and Page Down are passed through to the server.

For compatibility with [Emacs], all Alt-(key) combinations that are
not used by [mTCP] Telnet application are passed through as ESC-(key).

The Delete key is passed through as `ESC [ 3 ~` instead of the default
of character 127.

### Mouse

Mouse support is available in two forms.

First, if the server sends back `xterm` [DECSET] signals to enable
mouse tracking, mouse activity is transmitted to the server using the
`xterm` escape sequences.  [Emacs] sends these sequences when
`xterm-mouse-mode` is in use, and [Vim] sends them when `set mouse=a`
is used.

Second, if the [DECSET] signals are not received, mouse activity does
not result in escape sequences sent to the terminal, but the mouse can
be used for copying and pasting.  The controls are:

* Left-click and drag to select a region;
* Right-click to extend a selection;
* Double-click to select a word;
* Triple-click to select a line; and
* Middle-click to paste the selection.

Clicking the left and right mouse buttons at the same time emulates
the middle mouse button.

For mouse support to work, you need to run [MOUSE.COM] (which should
be available as part of your DOS system) before running [TELNET.EXE].

### Unicode

If UTF-8 characters are received, they are translated if possible to
[Code Page 437], but if no translation is available, `¿` is printed.
Translation does take place when sending keys to Telnet.

### Graphics

If a [Sixel graphics] escape sequence is encountered, the image will be
processed and displayed on the screen.  The user can then press any
key to get back to text mode.

For best results, use the [show] script on Linux to display images.
This script calls the `img2sixel` command with the correct parameters
for displaying images in MCGA or CGA graphics modes.

Display an image in 256 colors:

    show my_image.png

Take a picture with `/dev/video0` and then display it:

    show --me

Display an image in monochrome:

    show --mono my_mono_image.png

Display an image in monochrome if you have CGA graphics only:

    show --mono --cga my_mono_image.png

Display an image in a four-color CGA mode:

    show --colors CyanMagenta my_image.png
    show --colors LightCyanMagenta my_image.png
    show --colors GreenRed my_image.png
    show --colors LightGreenRed my_image.png

Show the usage instructions:

    show --help

## Compatibility

These changes have been tested on an IBM PS/2 Model 25 running MS-DOS
6.22 and Microsoft Mouse Driver ([MOUSE.COM]) version 7.03.  If your
machine does not have support for the "enhanced keyboard," try the
[TEL8088.EXE] file instead, which supports fewer keystrokes but should
work on older machines.  Try [TELNET.EXE] first and see if it works,
regardless of what machine you have.

Unfortunately, I do not have access to other machines in order to test
these changes.  Please feel free to create GitHub issues to let me
know what doesn't work.  EGA graphics modes will not be supported
unless someone with access to a computer with EGA graphics makes the
changes.

## Setup

On every Linux machines to which you plan to connect, copy the file
[ansi.src] to your home directory.

    curl -o ansi.src https://github.com/jhpyle/mTCP/blob/master/ansi.src

Then run:

    tic -x ansi.src

This will compile the [ansi.src] file and create the terminfo
description file `~/.terminfo/a/ansi`.  This will override the
standard terminfo file that is used when the `TERM` environment
variable is set to `ansi`.

If you have administrator access on the machine, it is better to
install the terminfo description file globally.  You can do this by
running:

    sudo tic -x -o/etc/terminfo ansi.src

Here, `/etc/terminfo` is the directory where custom terminfo
description files are installed.  It might be somewhere else on your
system.

After installing the terminfo file, you can run `rm ansi.src` because
you will not need the `ansi.src` file any longer (unless you would
like to edit it).

To improve your command line experience, add this to your `~/.inputrc`
file:

    "\e[7;5~" backward-kill-word
    "\e[3;2~" delete-char
    "\e[3;5~" kill-word
    "\e[5~": beginning-of-history
    "\e[6~": end-of-history

If you don't have an `~/.inputrc` file, you can create one with just
these lines in it.  This will allow you to type Ctrl-Backspace, Delete,
Ctrl-Delete, Page Up, and Page Down on the command line.

If you use [Emacs], download [ansi.el].

    curl -o ansi.src https://github.com/jhpyle/mTCP/blob/master/ansi.src

Incorporate the contents of this file into your `.emacs` file.  This
will ensure that [Emacs] will recognize the control sequences the the
[TELNET.EXE] application will send.

If you are using [Vim], add the following to your `.vimrc` file:

    if &term =~ "ansi"
      set mouse=a
    endif

However, I was not able to get [Vim] to respect this setting unless I
set `TERM=xterm`, which is not ideal because [mTCP]'s screen-updating
mechanism uses `ansi` sequences rather than `xterm` sequences.

Modify your `.bashrc` to add the following before the part that
references `force_color_prompt`:

    if [ "$TERM" = "ansi" ]; then
        force_color_prompt=yes
    fi

If you would prefer a monochrome prompt, leave out
`force_color_prompt=yes`.

If your computer does not support MCGA modes, set the following in
your `mtcp.cfg` file:

    TELNET_CGA 1

This is necessary for [Sixel graphics] to work appropriately.

The [show] script can be installed by running `./install.sh` as root.
The [install.sh] copies [show] to `/usr/local/bin` and installs color map
PNG files in `/usr/local/share/sixel`.

The [show] script depends on `libsixel-bin` and `streamer` (for the
`--me`) option.

    sudo apt-get install libsixel-bin streamer
    git clone https://github.com/jhpyle/mTCP
    cd mTCP
    sudo ./install.sh
    cd ..

If you use [R], you might want to use this shorthand for showing a
[ggplot2] image:

    sx <- function(){
      ggsave("graph.png", width=3.2, height=2.4)
      system("show graph.png")
    }

Add a `.mailcap` file to your home directory that contains:

    image/*; show %s

This will signal to other applications that the [show] command should
be used to display images.

If you are using [Lynx], you can set `USE_MOUSE:TRUE` in your
`/etc/lynx.cfg` file to enable mouse support.

You can also view images using [Sixel graphics] while using [Lynx].
You can press `*` to show links to images, or set
`MAKE_LINKS_FOR_ALL_IMAGES:TRUE` in your `/etc/lynx.cfg` file to turn
this feature on by default.  When you click on a link to an image,
your `.mailcap` file will be consulted and the [show] command will be
used to show the image.

You can also view images while using [Links].  To set this up, go to
the Setup menu (press Escape to open the menu bar) and go to
Associations.  Then you can add file associations for images.  The
first "Label" should be, e.g., "PNG," and the "Content-Type" should be
`image/png`, and "Program" should be `show %`.  Unselect "Block
terminal while program running" (this causes problems).  Make sure
that "Run on terminal" is selected.  Unselect "Ask before opening."
Don't select "Accepts HTTP URLs" or "Accepts FTP URLs."  Then save the
Association and repeat the process for `image/jpeg` and `image/gif`.
Then do "Save options" under "Setup."  This will create a file
`~/.links2/links.cfg`.  The file will contain something like the
following:

    association "PNG" "image/png" "show %" 11 1
    association "JPEG" "image/jpeg" "show %" 11 1
    association "GIF" "image/gif" "show %" 11 1

Image support is a little better in [Lynx] than it is on [Links]
because it will show a link to an image that is itself a hyperlink.

## Compiling

I compiled [TELNET.EXE] using [Open Watcom 1.9] inside [DosBox] on a Linux
machine.

I created a folder `~/dos` and installed Watcom under `~/dos/WATCOM`
and installed the [mTCP] source code under `~/dos/MTCP`.  I used the default
DosBox configuration (see `~/.dosbox`) with this at the end:

    [autoexec]
    # Lines in this section will be run at startup.
    # You can put your MOUNT lines here.
    @echo off
    mount c ~/dos
    SET PATH=C:\WATCOM\BINW
    SET WATCOM=C:\WATCOM
    SET INCLUDE=C:\WATCOM\H;C:\MTCP\TCPINC;C:\MTCP\INCLUDE
    C:\
    CD C:\MTCP\APPS\TELNET

The COMMAND.COM in [DosBox] limits command lines to 128 characters and
there does not appear to be a way to change this.  So I had to modify
the [MAKEFILE] by commenting out this line:

    compile_options += -i=$(tcp_h_dir) -i=$(common_h_dir)

In its place, I used the `INCLUDE` environment variable to let the
compiler know where the `.h` files are.  I also had to abbreviate
`TELNET.CFG` to `T.CFG` to squeeze some additional characters in.

Then I was able to compile Telnet by running `wmake` from the
`~/dos/MTCP/APPS/TELNET` directory.

I created a batch file called MAKE.BAT to recompile only the parts I was editing:

    del telnet.exe
    del telnet.obj
    del telnetsc.obj
    del telnetsx.obj
    del telnet.map
    del misc.obj
    wmake telnet.exe

This is much faster than waiting for a full `wmake` to complete.
(It's probably possible to do this with the Makefile itself, but I
forgot exactly how make works.)  For a final build you should run
`wmake` and then `wmake patch`.

[ansi.src]: https://github.com/jhpyle/mTCP/blob/master/ansi.src
[MAKEFILE]: https://github.com/jhpyle/mTCP/blob/master/MTCP/APPS/TELNET/MAKEFILE
[TELNET.EXE]: https://github.com/jhpyle/mTCP/blob/master/MTCP/APPS/TELNET/TELNET.EXE
[TEL8088.EXE]: https://github.com/jhpyle/mTCP/blob/master/MTCP/APPS/TELNET/TEL8088.EXE
[APPS/TELNET]: https://github.com/jhpyle/mTCP/blob/master/MTCP/APPS/TELNET
[mTCP]: https://www.brutman.com/
[ansi.el]: https://github.com/jhpyle/mTCP/blob/master/ansi.el
[DECSET]: https://invisible-island.net/xterm/ctlseqs/ctlseqs.pdf
[Code Page 437]: https://en.wikipedia.org/wiki/Code_page_437
[MOUSE.COM]: https://www.computerhope.com/issues/ch000007.htm
[Open Watcom 1.9]: https://sourceforge.net/projects/openwatcom/files/open-watcom-1.9/
[DosBox]: https://www.dosbox.com/
[Vim]: https://www.vim.org/
[Emacs]: https://www.gnu.org/software/emacs/
[Sixel graphics]: https://en.wikipedia.org/wiki/Sixel
[show]: https://github.com/jhpyle/mTCP/blob/master/sixel/show
[install.sh]: https://github.com/jhpyle/mTCP/blob/master/install.sh
[R]: https://www.r-project.org/
[ggplot2]: https://ggplot2.tidyverse.org/
[Links]: https://en.wikipedia.org/wiki/Links_(web_browser)
[Lynx]: https://en.wikipedia.org/wiki/Lynx_(web_browser)
