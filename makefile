.PHONY : all clean
all=pterm
CFLAGS=-O3 -DNDEBUG
LIBS=-lm

pterm: main.o
	gcc -o $@ $^ $(LIBS)

clean:
	rm -rf *.o pterm