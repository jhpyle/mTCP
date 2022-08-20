wmake config=utf.h
wmake patch
copy telnet.exe telnet.tmp
wmake config=noutf.h
wmake patch
copy telnet.exe telnetnu.exe
copy telnet.tmp telnet.exe
erase telnet.tmp
