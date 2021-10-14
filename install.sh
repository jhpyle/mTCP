#! /bin/bash

# Installs the "show" executable and its color map files in /usr/local/bin on
# your Linux system, and installs the 'ansi' termcap file.
#
#     git clone https://github.com/jhpyle/mTCP
#     cd mTCP
#     sudo ./install.sh

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi
install ./sixel/show /usr/local/bin \
&& install ./printing/tprint /usr/local/bin \
&& install ./printing/stopprint /usr/local/bin \
&& install ./printing/startprint /usr/local/bin \
&& mkdir -p /usr/local/share/sixel \
&& install -m 644 ./sixel/*.png /usr/local/share/sixel \
&& echo "Installed the 'show' command." \
&& tic -x ./ansi.src \
&& echo "Installed the 'ansi' termcap file."
