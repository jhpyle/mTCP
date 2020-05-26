#! /bin/bash

# Installs the "show" executable and its color map files in /usr/local/bin on
# your Linux system.
#
#     git clone https://github.com/jhpyle/mTCP
#     cd mTCP
#     sudo ./install.sh

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi

install ./sixel/show /usr/local/bin \
&& mkdir -p /usr/local/share/sixel \
&& install -m 644 ./sixel/*.png /usr/local/share/sixel
