#CFLAGS=-fomit-frame-pointer -O3 -pthread -std=c99 -m64 -msse4.2 -W -Wall
# This is actually faster on my machine:
CFLAGS=-O2 -pthread -std=c99 -m64 -W -Wall

all: noelkdf memorycpy

noelkdf: main.c noelkdf-ref.c noelkdf.h sha256.c sha256.h
	gcc $(CFLAGS) -pthread main.c noelkdf-ref.c sha256.c -o noelkdf

memorycpy: memorycpy.c
	gcc $(CFLAGS) -pthread memorycpy.c -o memorycpy
