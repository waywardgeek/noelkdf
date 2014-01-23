all: noelkdf memorycpy

noelkdf: main.c noelkdf-ref.c noelkdf.h sha256.c sha256.h
	gcc -Wall -m64 -O3 -pthread main.c noelkdf-ref.c sha256.c -o noelkdf

memorycpy: memorycpy.c
	gcc -Wall -m64 -O3 -pthread memorycpy.c -o memorycpy
