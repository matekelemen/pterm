.PHONY : all clean
all=pterm
CFLAGS=-O3 -DNDEBUG -march=native -funsafe-math-optimizations
LIBS=-lm

GCC_VERSION := $(shell gcc -dumpversion)
ifeq ($(GCC_VERSION),13.2.1)
  $(error Some random incompatibility with your OS)
endif

pterm: main.o
	cc -o $@ $^ $(LIBS)

clean:
	rm -rf *.o pterm
