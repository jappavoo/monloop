O          := 0
CFLAGS     += -g -O${O} -std=gnu99 -MD -MP -Wall
CXXFLAGS   += -g -O${O} -MD -MP -Wall
LDFLAGS    += -lpthread

TARGETS := monlooptest c++monlooptest

all: ${TARGETS}

monloop.o: monloop.c 
	${CC} ${CFLAGS} -c -o $@ $^ 

monlooptest: monlooptest.c monloop.o
	${CC} ${CFLAGS} -o $@ $^ ${LDFLAGS}

c++monlooptest.o: c++monlooptest.cpp
	${CXX} ${CXXFLAGS} -c -o $@ $^ 

c++monlooptest: c++monlooptest.o monloop.o
	${CXX} ${CXXFLAGS} -o $@ $^ ${LDFLAGS}

clean:
	-rm -rf $(wildcard *.o *.d ${TARGETS})
