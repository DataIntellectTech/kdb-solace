CXX:=gcc 

LIBS= -Lsolapi/lib

FLAGS= -shared -DKXVER=3 -fPIC  -D_LINUX_X86_64 -lpthread

IGNORE_WARN = -Wno-discarded-qualifiers -Wno-incompatible-pointer-types

DEBUGFLAGS= -g

LINKFLAGS= -lsolclient

INCLUDE=  -Iinclude

OUT= -o lib/solace.so

FILES= src/solace.c src/solace_k.c src/cb.c

LINKS = -lsolclient -lrt

OTHER = -DPROVIDE_LOG_UTILITIES 

COMPILE=$(CXX) ${LIBS} ${FLAGS} ${INCLUDE} ${FILES} ${OUT}

all:  build

build: clean prod 

prod:
	 $(CXX) ${LIBS}  ${FLAGS} ${LINKFLAGS} ${INCLUDE} ${OTHER} ${FILES} ${OUT} ${LINKS} ${IGNORE_WARN}
clean: 
	if ! test -d lib;then mkdir lib/;fi
	rm -f        lib/solace.*
debug: 
	$(CXX) ${DEBUGFLAGS} ${LIBS}  ${FLAGS} ${LINKFLAGS} ${INCLUDE} ${OTHER} ${FILES} ${OUT} ${LINKS}
 


