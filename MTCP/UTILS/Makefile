
all : clean ptach.exe

clean : .symbolic
  rm -f ptach.exe
  rm -f *.obj
  rm -f *.map

ptach.exe : patch.cpp
  wcl386 patch.cpp
  copy patch.exe ptach.exe
