# Atividade: Modelagem de Algoritmos Paralelos — Bag-of-Tasks com pthreads
CC      = gcc
CFLAGS  = -Wall -O2
LDFLAGS = -pthread

PROGS = ex0_fila ex1_estatico ex2_bag

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(PROGS)

.PHONY: all clean
