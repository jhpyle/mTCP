# jhpyle/mTCP

This is an unofficial fork of [mTCP] by Michael B. Brutman.

This fork makes the Telnet application in [mTCP] more usable when
connecting to Linux machines.  It adds:

* [Enhanced Keyboard] support
* Mouse and clipboard support
* Translation of incoming Unicode characters
* [Sixel graphics]

The recompiled binary is available as the [TELNET.EXE] file and the
[TELNET88.EXE] file, which are in the [bin] directory.  The
[TELNET88.EXE] file is for machines without [Enhanced Keyboard] support.

The fork is based on Mr. Brutman's `mTCP-src_2020-03-07.zip`.

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
Down keys (or Ctrl-Page Up and Ctrl-Page Dn with [TELNET88.EXE]), and
Page Up and Page Down are passed through to the server.

For compatibility with [Emacs], all Alt-(key) combinations that are
not used by [mTCP] Telnet application are passed through as ESC-(key).

The Delete key is passed through as `ESC [ 3 ~` instead of the default
of character 127.

Some of the above keys are not available if your computer lacks
support for the [Enhanced Keyboard].

### Mouse

Mouse support is available in two forms.

First, if the server sends back `xterm` [DECSET] signals to enable
mouse tracking, mouse activity is transmitted to the server using the
`xterm` escape sequences.  [Emacs] sends these sequences when
`xterm-mouse-mode` is in use, and [Vim] sends them when `set mouse=a`
is used.  Midnight Commander provides mouse support if the `-x` switch
is used to enable `xterm` mode.

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
be available as part of your DOS system) before running [TELNET.EXE]
or [TELNET88.EXE].

### Unicode

If UTF-8 characters are received, they are translated if possible to
one of the 256 characters that can be displayed in text mode.  If no
translation is available, a default character is printed.  Translation
does not take place when sending outbound characters.

By default, Unicode characters are translated into [Code Page 437] and
the default character for an untranslatable Unicode character is `¿`.

Unicode translation can be configured by adding directives to your
`MTCP.CFG` file such as the following, which will enable [Code Page
737] instead of [Code Page 437]:

    TELNET_CODEPAGE 737
    TELNET_UTF_DEFAULT 0xb7
    TELNET_UTF 737 0x0000 0x00
    TELNET_UTF 737 0x0001 0x01
    TELNET_UTF 737 0x0002 0x02
    TELNET_UTF 737 0x0003 0x03
    ...
    TELNET_UTF 737 0x00a0 0xff

There are example configurations in the [`config`] folder for [Code
Page 737], [Code Page 775], [Code Page 850], [Code Page 852], [Code
Page 855], [Code Page 857] [Code Page 860], [Code Page 861], [Code
Page 862], [Code Page 863], [Code Page 864], [Code Page 865], [Code
Page 866], [Code Page 869], and [Code Page 874].  A file is also
included for [Code Page 437] in case you want to customize the default
translation.

You can copy and paste from these files into your `MTCP.CFG` file.
The `TELNET_CODEPAGE` directive selects a code page, and any
directives beginning with `TELNET_UTF 737` will be used to define the
translations that are used.  The numbers must be written in
hexadecimal.  The format of each line is:

    TELNET_UTF <CODE_PAGE_NUMBER> <UNICODE_NUMBER> <CHARACTER_NUMBER>

where `CODE_PAGE_NUMBER` is the number of the code page (e.g. 737),
the `UNICODE_NUMBER` is a hexadecimal Unicode number (e.g., `0x266a` for
the musical note character) and `CHARACTER_NUMBER` is a hexadecimal
8-bit number representing the character on the screen (e.g., `0x0d` for
the musical note character).

The `TELNET_UTF_DEFAULT` directive indicates which character should be
printed if there is no translation for a particular Unicode character.
The number must be written in hexadecimal.  The default is `0xa8`,
which is the character ¿.

If you switch between code pages, you can keep definitions for several
code pages in your `MTCP.CFG` file and change only the
`TELNET_CODEPAGE` line when you want to switch among them.

Depending on what applications you use and what content you view, you
may wish to modify the standard Unicode translations, add additional
characters, or add additional code pages.

The code page translations in the [`config`] folder were created using
translation tables available on [unicode.org].

### Graphics

If a [Sixel graphics] escape sequence is encountered, the image will
be processed and displayed on the screen.  After viewing the image,
the user can then press any key to get back to text mode.

The supported graphics modes are:

* 720x348x2 - requires a [Hercules Graphics Card]
* 320x200x256 - requires [VGA] or [MCGA]
* 640x480x2 - requires [VGA] or [MCGA]
* 320x200x4 - [CGA]
* 640x200x2 - [CGA]

Telnet will use the highest resolution graphics mode available, given
the number of colors in the Sixel image.  If there are two colors, it
will use a two-color graphics mode.  If there are four colors, it will
use a 320x200x4 mode with the matching palette.

Telnet does not autodetect whether you have a [Hercules Graphics Card]
or [VGA] or [MCGA].  By default, it assumes you have [VGA] or [MCGA]
and it will try to display in the [VGA]/[MCGA] modes.

If your computer does not support the [VGA]/[MCGA] modes, you will
need to add a line to your `MTCP.CFG` file to stop it from using those
modes.  If you have a [CGA] adapter, set the following:

    TELNET_CGA 1

If you have a [Hercules Graphics Card], set the following instead:

    TELNET_HGC 1

Telnet will not do any image resizing or dithering.  If an image is
wider or taller than the graphics mode supports, then it will not
display.

For best results, telnet to a Linux machine and use the included
[show] script on Linux to send Sixel escape sequences back to your
Telnet client.  This script calls the [`img2sixel`] command with the
appropriate parameters for resizing images and converting colors.

Without any parameters, [show] will display an image in 256 colors.

    show my_image.png

With the `--me` parameter, [show] will take a picture with
`/dev/video0` and then display it:

    show --me

If you have the [Hercules Graphics Card], use the `--hercules` switch:

    show --hercules my_mono_image.png

If you have [CGA] graphics only, use the `--cga` switch so that color
images are reduced to four colors.

    show --cga my_image.png

By default, [show] will use the bright cyan/magenta/white palette in
`--cga` mode.  You can tell it to use a different palette:

    show --colors CyanMagenta my_image.png
    show --colors LightCyanMagenta my_image.png
    show --colors GreenRed my_image.png
    show --colors LightGreenRed my_image.png

To make an image display in monochrome, use the `--mono` switch:

    show --mono my_mono_image.png

By default, the `--mono` switch will resize the image to fit in
640x480, which [VGA] and [MCGA] monitors can display.  If you have
[CGA] only, make sure to include the `--cga` switch:

    show --mono --cga my_mono_image.png

This will resize the image to fit within [CGA]'s 640x200 resolution.

To see the usage instructions, call `show` with the `--help` switch.

    show --help

For instructions on installing the [show] utility, see the Setup
section below.

## Compatibility

There are two versions of the executable:

* TELNET.EXE - for machines with [Enhanced Keyboard] support ([IBM
  PC/AT] dated 11/15/1985 and after, [IBM PC/XT] dated 1/10/1986 and
  after, [IBM XT 286], and [PS/2] models.
* TELNET88.EXE - for earlier machines without [Enhanced Keyboard]
  support (e.g., [IBM PC 5150]).

This package has been tested on:

* [IBM PS/2 Model 25] with an 8086 processor running PC-DOS 4.0 and
  Microsoft Mouse Driver ([MOUSE.COM]) version 7.03.
* [IBM PC model 5150] with the [Hercules Graphics Card].

Unfortunately, I do not have access to other machines, so I do not
know what problems may arise on other systems.  Please feel free to
create GitHub issues to let me know what doesn't work.  [EGA] graphics
modes will not be supported unless someone with access to a computer
with [EGA] graphics makes the changes.

As discussed above, you will need to add `TELNET_HGC 1` or
`TELNET_CGA 1` to your `MTCP.CFG` file if you have a [Hercules
Graphics Card] or [CGA] adapter in your system.

## Setup

In order for remote machines to communicate with your Telnet client
using escape sequences for colors, mouse support, and keypresses, the
remote machines will need to know the precise capabilities of your
Telnet client.  On Linux, terminal capabilites are defined in a
"termcap" file.  The `TERM` variable in your remote shell environment
tells the remote machine which "termcap" file to use.  `TERM` is
typically set to a value like `ansi`, `vt320`, or `xterm`.
Terminal-based applications use the `TERM` variable to decide what
features will be enabled and how they will communicate with you.

The mTCP Telnet client reports its terminal type as `ANSI`.  This can
be changed by setting the variable `TELNET_TERMTYPE` in the `MTCP.CFG`
file, but it is generally better to keep it as `ANSI`.  Many
terminal-based applications do not support non-standard terminal
types, so you are generally better off using a standard terminal type
like `ANSI`.

The `ANSI` standard is not well-defined.  There are many variants, and
the one that mTCP Telnet uses is its own variant.  In order to use all
of the features of mTCP Telnet's ANSI terminal emulation, you need to
create your own "termcap" file.  This package contains the source code
for this "termcap" file ([ansi.src]).

On every machine to which you might want to connect (directly with
Telnet or indirectly with [ssh]), install the `ansi` "termcap" file
and the [show] script.

You can do this by cloning the GitHub repository and running
`./install.sh` as root.  If you don't have `git` installed, you can
install it with `sudo apt-get install git`.

The [install.sh] script copies [show] to `/usr/local/bin`, installs
color map PNG files in `/usr/local/share/sixel`, and compiles and
installs the `ansi` "termcap" file.

The [show] script depends on `libsixel-bin` (for `img2sixel`),
`libimage-size-perl` (for `imgsize`), and `streamer` (for the `--me`)
option.  The installation of the `ansi` "termcap" file depends on the
`tic` command, which is available in `ncurses-bin`.

    sudo apt-get install libsixel-bin libimage-size-perl streamer ncurses-bin
    git clone https://github.com/jhpyle/mTCP
    cd mTCP
    sudo ./install.sh
    cd ..

The names of these packages and the application for installing
packages (in this example, `apt-get`) might be different on your
machines, so you may need to revise this.

If `/usr/local/bin` is not part of your `PATH` (try `echo $PATH` to
see what your `PATH` is) you might want to edit the install script so
that it installs the `show` script elsewhere.

### Manual installation

If you want to install the [show] script manually, run commands
equivalent to the following:

    git clone https://github.com/jhpyle/mTCP
    sudo install mTCP/sixel/show /usr/local/bin
    sudo mkdir -p /usr/local/share/sixel
    sudo install -m 644 mTCP/sixel/*.png /usr/local/share/sixel

The [show] script needs to go into a directory that is in your `PATH`.
The PNG files can go in any directory, but if you put them in a
directory other than `/usr/local/share/sixel`, make sure you edit the
[show] script to change the definition of `COLORMAPS` so that it
points to the directory you are using.

To install the `ansi` "termcap" file manually, copy the file
[ansi.src] to the current directory.

    curl -o ansi.src https://github.com/jhpyle/mTCP/blob/master/ansi.src

Then run:

    sudo tic -x -o/etc/terminfo ansi.src

Here, `/etc/terminfo` is the directory where custom terminfo
description files are installed.  It might be located in a different
directory on your system.

This will override the standard terminfo file that is used when the
`TERM` environment variable is set to `ansi`.

If you do not have administrator access on the machine, you can
install the terminfo description for yourself only.  You can do this
by running:

    tic -x ansi.src

This will compile the [ansi.src] file and create the terminfo
description file `~/.terminfo/a/ansi`.

After installing the terminfo file, you can run `rm ansi.src` because
you will not need the `ansi.src` file any longer (unless you would
like to edit it).

### Configuring your applications

The "termcap" file, on its own, is not sufficient for all of your
applications to work appropriately with the mTCP Telnet client.  Many
applications look at the `TERM` environment variable but bypass the
"termcap" system.  You will need to edit the configuration files of
your applications to get the most out of mTCP Telnet.

To improve your [bash] command line experience, add this to your
`~/.inputrc` file:

    "\e[7;5~" backward-kill-word
    "\e[3;2~" delete-char
    "\e[3;5~" kill-word
    "\e[5~": beginning-of-history
    "\e[6~": end-of-history

If you don't have an `~/.inputrc` file, you can create one with just
these lines in it.  This will allow you to type Ctrl-Backspace, Delete,
Ctrl-Delete, Page Up, and Page Down on the command line.

If you use [Emacs], download [ansi.el].

    curl -o ansi.el https://github.com/jhpyle/mTCP/blob/master/ansi.el

Incorporate the contents of this file into your `.emacs` file.  This
will ensure that [Emacs] will recognize the control sequences that the
Telnet client will send.

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
your `~/.mailcap` file will be consulted and the [show] command will be
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

If you use Midnight Commander, you can edit the file
`/usr/lib/mc/ext.d/image.sh` and change this line:

    ("${MC_XDG_OPEN}" "${MC_EXT_FILENAME}" >/dev/null 2>&1) || \

to:

    ("${MC_XDG_OPEN}" "${MC_EXT_FILENAME}") || \

Then, if your `~/.mailcap` has been set up as discussed above, the
`xdg-open` command will run `show` in the terminal when you open an
image file.

If you use [R], you might want to use this shorthand for showing a
[ggplot2] image:

    sx <- function(){
      ggsave("graph.png", width=3.2, height=2.4)
      system("show graph.png")
    }

## Compiling

I compiled [TELNET.EXE] and [TELNET88.EXE] using [Open Watcom 1.9]
inside [DosBox] on a Linux machine.

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

I created a batch file called `MAKE.BAT` to recompile only the parts I was editing:

    del telnet.exe
    del telnet.obj
    del telnetsc.obj
    del telnetsx.obj
    del telnet.map
    del misc.obj
    wmake telnet.exe config=T.CFG

This is much faster than waiting for a full `wmake` to complete.

To build a version of `telnet.exe` without [Enhanced Keyboard]
support, use the `OL.CFG` file instead of `T.CFG`.

For a final build, run `wmake` and then `wmake patch`, and generate
both `TELNET88.EXE` and `TELNET.EXE`.  (See the file `MAKEALL.BAT`)

[ansi.src]: https://github.com/jhpyle/mTCP/blob/master/ansi.src
[MAKEFILE]: https://github.com/jhpyle/mTCP/blob/master/MTCP/APPS/TELNET/MAKEFILE
[TELNET.EXE]: https://github.com/jhpyle/mTCP/blob/master/MTCP/APPS/TELNET/TELNET.EXE
[TELNET88.EXE]: https://github.com/jhpyle/mTCP/blob/master/MTCP/APPS/TELNET/TELNET88.EXE
[bin]: https://github.com/jhpyle/mTCP/blob/master/bin
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
[unicode.org]: https://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/PC/
[`config`]: https://github.com/jhpyle/mTCP/blob/master/config
[Code Page 437]: https://en.wikipedia.org/wiki/Code_page_437
[Code Page 737]: https://en.wikipedia.org/wiki/Code_page_737
[Code Page 775]: https://en.wikipedia.org/wiki/Code_page_775
[Code Page 850]: https://en.wikipedia.org/wiki/Code_page_850
[Code Page 852]: https://en.wikipedia.org/wiki/Code_page_852
[Code Page 855]: https://en.wikipedia.org/wiki/Code_page_855
[Code Page 857]: https://en.wikipedia.org/wiki/Code_page_857
[Code Page 860]: https://en.wikipedia.org/wiki/Code_page_860
[Code Page 861]: https://en.wikipedia.org/wiki/Code_page_861
[Code Page 862]: https://en.wikipedia.org/wiki/Code_page_862
[Code Page 863]: https://en.wikipedia.org/wiki/Code_page_863
[Code Page 864]: https://en.wikipedia.org/wiki/Code_page_864
[Code Page 865]: https://en.wikipedia.org/wiki/Code_page_865
[Code Page 866]: https://en.wikipedia.org/wiki/Code_page_866
[Code Page 869]: https://en.wikipedia.org/wiki/Code_page_869
[Code Page 874]: https://en.wikipedia.org/wiki/Code_page_874
[Hercules Graphics Card]: https://en.wikipedia.org/wiki/Hercules_Graphics_Card
[VGA]: https://en.wikipedia.org/wiki/Video_Graphics_Array
[MCGA]: https://en.wikipedia.org/wiki/Multi-Color_Graphics_Array
[`img2sixel`]: https://github.com/saitoha/libsixel
[CGA]: https://en.wikipedia.org/wiki/Color_Graphics_Adapter
[Enhanced Keyboard]: https://en.wikipedia.org/wiki/IBM_PC_keyboard
[IBM PC 5150]: https://en.wikipedia.org/wiki/IBM_Personal_Computer
[IBM PC model 5150]: https://en.wikipedia.org/wiki/IBM_Personal_Computer
[IBM PC/XT]: https://en.wikipedia.org/wiki/IBM_Personal_Computer_XT
[IBM PC/AT]: https://en.wikipedia.org/wiki/IBM_Personal_Computer/AT
[IBM XT 286]: https://en.wikipedia.org/wiki/IBM_Personal_Computer_XT#IBM_XT_286
[PS/2]: https://en.wikipedia.org/wiki/IBM_PS/2
[IBM PS/2 Model 25]: https://en.wikipedia.org/wiki/IBM_PS/2
[EGA]: https://en.wikipedia.org/wiki/Enhanced_Graphics_Adapter
[ssh]: https://en.wikipedia.org/wiki/Secure_Shell
[bash]: https://en.wikipedia.org/wiki/Bash_(Unix_shell)
