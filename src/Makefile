CC=gcc
CFLAGS=-I. -Wall
IDIR=.
LDFLAGS=
SOURCES=serial.c main.c
OBJ= $(SOURCES: .cpp=.o)
EXE=cwlinux

all: $(SOURCES) $(EXE)

$(EXE): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $@

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)


.PHONY: clean

clean:
	rm -f *.o *~ core  $(EXE)
