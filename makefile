.PHONY : all clean
all=pterm
CFLAGS=-O3 -DNDEBUG
LIBS=-lm

pterm: pterm.o
	gcc -o $@ $^ $(LIBS)

clean:
	rm -rf *.o pterm