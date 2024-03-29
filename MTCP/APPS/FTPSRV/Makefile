#
# FTPSRV makefile
#
#
# Possible optimizations for 8088 class processors
#
# -oa   Relax alias checking
# -ob   Try to generate straight line code
# -oe - expand user functions inline (-oe=20 is default, adds lots of code)
# -oh   Enable repeated optimizations
# -oi   generate certain lib funcs inline
# -oi+  Set max inline depth (C++ only, use -oi for C)
# -ok   Flowing of register save into function flow graph
# -ol   loop optimizations
# -ol+  loop optimizations plus unrolling
# -or   Reorder for pipelined (486+ procs); not sure if good to use
# -os   Favor space over time
# -ot   Favor time over space
# -ei   Allocate an "int" for all enum types
# -zp2  Allow compiler to add padding to structs
# -zpw  Use with above; make sure you are warning free!
# -0    8088/8086 class code generation
# -s    disable stack overflow checking
# -zmf  put each function in a new code segment; helps with linking

# This code is big by nature, so throw in a few more options to improve
# performance.  We don't care about space so use -oe too.


tcp_h_dir = ..\..\TCPINC\
tcp_c_dir = ..\..\TCPLIB\
common_h_dir = ..\..\INCLUDE

memory_model = -ml
#compile_options = -0 $(memory_model) -DCFG_H="ftpsrv.cfg" -oh -ok -ot -s -oa -ei -zp2 -zpw -we -ob -ol+ -oi+ -oe
compile_options = -0 $(memory_model) -DCFG_H="ftpsrv.cfg" -oh -ok -ot -s -oa -ei -zp2 -zpw -we -ob -ol+ -oi+
compile_options += -i=$(tcp_h_dir) -i=$(common_h_dir)

tcpobjs = packet.obj arp.obj eth.obj ip.obj tcp.obj tcpsockm.obj  utils.obj timer.obj ipasm.obj trace.obj
objs = ftpsrv.obj ftpcl.obj ftpusr.obj

all : clean ftpsrv.exe 

clean : .symbolic
  rm -f ftpsrv.exe
  rm -f *.obj
  rm -f *.map

patch : .symbolic
  ..\..\utils\ptach ftpsrv.exe ftpsrv.map $(memory_model)

.asm : $(tcp_c_dir)

.cpp : $(tcp_c_dir)

.asm.obj :
  wasm -0 $(memory_model) $[*

.cpp.obj :
  wpp $[* $(compile_options)

ftpsrv.exe : $(tcpobjs) $(objs)
  wlink system dos option map option eliminate option stack=4096 name $@ file *.obj
