# jhpyle/mTCP

This is an unofficial fork of [mTCP] by Michael B. Brutman.

This fork makes minor changes to the Telnet application to extend the
features of the "ANSI" terminal emulation in order to make Telnet more
usable for connecting to Linux machines.

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

For compatibility with Emacs, all Alt-(key) combinations that are not
used by [mTCP] Telnet application are passed through as ESC-(key).

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

## Compatibility

These changes have been tested on an IBM PS/2 Model 25 running MS-DOS
6.22 and Microsoft Mouse Driver ([MOUSE.COM]) version 7.03.  If your
machine does not have support for the "enhanced keyboard," try the
[TEL8088.EXE] file instead, which supports fewer keystrokes but should
work on older machines.

Unfortunately, I do not have access to other machines in order to test
these changes.  Please feel free to create GitHub issues to let me
know what doesn't work.

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

If you are using Emacs, download [ansi.el].

    curl -o ansi.src https://github.com/jhpyle/mTCP/blob/master/ansi.src

Incorporate the contents of this file into your `.emacs` file.  This
will ensure that Emacs will recognize the control sequences the the
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
        export LC_ALL=C
        force_color_prompt=yes
    fi

If you would prefer a monochrome prompt, leave out
`force_color_prompt=yes`.

Things might work fine without the `LC_ALL=C`.  It helps to turn off
Unicode characters that might render as `¿`.

Instead of `LC_ALL=C`, you could try `LC_ALL=en_US.ISO-8859-1`.  Note
that you may first need to run `sudo dpkg-reconfigure locales` and
install the `en_US.ISO-8859-1` locale.  The `ISO-8859-1` character set
supports the "extended ASCII" codes 128-255, which the PC can display.
Applications that support this character set include:

* Midnight Commander
* `tree -S`
* Nethack with the `symset` set to `IBMGraphics`.

Not all applications will respect the `LC_ALL` environment variable.
Some applications might still send Unicode into your terminal.

Depending on the application, it might be perfectly fine to refrain
from setting `LC_ALL` and allowing a default locale (e.g.,
`en_US.utf8`) to send Unicode to the Telnet application.  The
translation between Unicode and [Code Page 437] may be sufficient.

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
compiler know where the `.h` files are.

Then I was able to compile Telnet by running `wmake` from the
`~/dos/MTCP/APPS/TELNET` directory.

I created a batch file called MAKE.BAT to recompile the parts I was editing:

    del telnet.exe
    del telnet.obj
    del telnet.map
    del mouse.obj
    wmake telnet.exe

This is much faster than waiting for a full `wmake` to complete.

[ansi.src]: https://github.com/jhpyle/mTCP/blob/master/ansi.src
[MAKEFILE]: https://github.com/jhpyle/mTCP/blob/master/MTCP/APPS/TELNET/MAKEFILE
[TELNET.EXE]: https://github.com/jhpyle/mTCP/blob/master/MTCP/APPS/TELNET/TELNET.EXE
[TEL8088.EXE]: https://github.com/jhpyle/mTCP/blob/master/MTCP/APPS/TELNET/TEL8088.EXE
[APPS/TELNET]: https://github.com/jhpyle/mTCP/blob/master/MTCP/APPS/TELNET
[mTCP]: https://www.brutman.com/
[ansi.el]: https://github.com/jhpyle/mTCP/blob/master/ansi.el
[DECSET]: https://invisible-island.net/xterm/ctlseqs/ctlseqs.pdf
[Code Page 437]: https://en.wikipedia.org/wiki/Code_page_437
[MOUSE.COM mouse driver]: https://www.computerhope.com/issues/ch000007.htm
[Open Watcom 1.9]: https://sourceforge.net/projects/openwatcom/files/open-watcom-1.9/
[DosBox]: https://www.dosbox.com/
[Vim]: https://www.vim.org/
