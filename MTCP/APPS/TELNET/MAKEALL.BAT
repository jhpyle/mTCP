wmake config=utf.h
wmake patch
ren telnet.exe telnet.tmp
wmake config=noutf.h
wmake patch
ren telnet.exe telnetnu.exe
ren telnet.tmp telnet.exe
