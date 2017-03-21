ifndef $(VER)
  VER=32
  ifeq ($(shell uname -m),x86_64)
    VER=64
  endif
endif
all: client serverT serverP

serverT: serverT.cpp semafor.cpp msg_io.cpp
	g++ serverT.cpp semafor.cpp msg_io.cpp -o serverT -lpthread
serverP: serverP.cpp semafor.cpp msg_io.cpp
	g++ serverP.cpp semafor.cpp msg_io.cpp -o serverP
client: filozof_cl.cpp msg_io.cpp protocol.o$(VER)
	g++ $^ -o $@

