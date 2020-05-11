# jhpyle/mTCP

This is an unofficial fork of [mTCP] by Michael B. Brutman.

This fork makes minor changes to the Telnet application to extend the
features of the "ANSI" terminal emulation in order to make Telnet more
usable for connecting to Linux machines.

The recompiled binary is available as the [TELNET.EXE] file, which is
in APPS/TELNET directory.  It is based on Mr. Brutman's
`mTCP-src_2020-03-07.zip`.

## The changes

The purpose of the repository is to expand the capabilities of [mTCP]
Telnet's `ansi` terminal emulator.  Where a feature was not supported
by (what appears to be) the `ansi` standard, escape sequences were
borrowed from `xterm`.  The exception is Ctrl-backspace, where there
was no standard, so a custom escape sequence was used.

Keys that are available are:

* Function keys F1-F12;
* Insert, Delete, Home, End, Page Up, Page Down;
* Cursor keys;
* Shift, Ctrl, Ctrl-shift, Alt, and Alt-Shift modifications of the above;
* Tab, Ctrl-Tab, Shift-Tab;
* Ctrl-Backspace; 
* Ctrl-spacebar;
* Ctrl-PrtScr.

In the official [mTCP], Page Up and Page Down scroll are not passed
through but are used for scrolling through the output history.  In
this version, that function is moved to the Alt-Page Up and Alt-Page
Down keys. Page Up and Page Down are now passed through to the server.

For compatibility with Emacs, all Alt-(key) combinations that are not
used by [mTCP] Telnet application are passed through as ESC-(key).

The Delete key is passed through as `ESC [ 3 ~` instead of the default
of character 127.

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

Modify your `.bashrc` to add this to the part before the part that
references `force_color_prompt`:

    if [ "$TERM" = "ansi" ]; then
        export LC_ALL=C
        force_color_prompt=yes
    fi

If you would prefer a monochrome prompt, leave out
`force_color_prompt=yes`.

The `LC_ALL=C` part is very important because it will help to turn off
Unicode characters that would not render correctly.

Alternatively, you can use `LC_ALL=en_US.ISO-8859-1`, but in order to
do so you will first need to run `sudo dpkg-reconfigure locales` and
install the `en_US.ISO-8859-1` locale.  The `ISO-8859-1` character set
includes the "extended ASCII" codes 128-255, which the PC can display.
Applications that support this character set include:

* Midnight Commander
* `tree -S`
* Nethack with the `symset` set to `IBMGraphics`.

Not all applications will respect the `LC_ALL` environment variable.
Some applications might still send Unicode into your terminal.

## Compiling

I compiled [TELNET.EXE] using Open Watcom 1.9 inside DosBox on a Linux
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

The COMMAND.COM in DosBox limits command lines to 128 characters and
there does not appear to be a way to change this.  So I had to modify
the [MAKEFILE] by commenting out this line:

    compile_options += -i=$(tcp_h_dir) -i=$(common_h_dir)

In its place, I used the `INCLUDE` environment variable to let the
compiler know where the `.h` files are.

Then I was able to compile Telnet by running `wmake` from the
`~/dos/MTCP/APPS/TELNET` directory.

[ansi.src]: https://github.com/jhpyle/mTCP/blob/master/ansi.src
[MAKEFILE]: https://github.com/jhpyle/mTCP/blob/master/APPS/TELNET/MAKEFILE
[TELNET.EXE]: https://github.com/jhpyle/mTCP/blob/master/APPS/TELNET/TELNET.EXE
[mTCP]: https://www.brutman.com/
[ansi.el]: https://github.com/jhpyle/mTCP/blob/master/ansi.el
