.PHONY : all clean
all=pterm
CFLAGS=-O3 -DNDEBUG -march=native -funsafe-math-optimizations
LIBS=-lm

pterm: main.o
	cc -o $@ $^ $(LIBS)

clean:
	rm -rf *.o pterm
