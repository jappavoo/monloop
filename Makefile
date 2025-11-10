O          := 2
CFLAGS     += -g -O${O} -std=gnu99 -MD -MP -Wall
LDFLAGS    += -lpthread

TARGETS := monloop.o monlooptest

all: ${TARGETS}

monloop.o: monloop.c
	${CC} ${CFLAGS} -c -o $@ $^ 

monlooptest: monlooptest.c monloop.o
	${CC} ${CFLAGS} -o $@ $^ ${LDFLAGS}

clean:
	-rm -rf $(wildcard *.o *.d ${TARGETS})
