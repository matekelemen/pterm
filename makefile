.PHONY : all clean
all = pterm
LIBS = -lm

pterm: pterm.o
	gcc -o $@ $^ $(LIBS)

clean:
	rm -rf *.o pterm