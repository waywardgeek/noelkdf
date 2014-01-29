#CFLAGS=-O2 -std=c99 -m64 -W -Wall
CFLAGS=-g -std=c99 -m64 -W -Wall

all: noelkdf-ref noelkdf memorycpy

noelkdf-ref: main.c noelkdf-ref.c noelkdf.h sha256.c sha256.h
	gcc $(CFLAGS) main.c noelkdf-ref.c sha256.c -o noelkdf-ref

noelkdf: main.c noelkdf-pthread.c noelkdf.h sha256.c sha256.h
	gcc $(CFLAGS) -pthread main.c noelkdf-pthread.c sha256.c -o noelkdf

memorycpy: memorycpy.c
	gcc $(CFLAGS) -pthread memorycpy.c -o memorycpy
