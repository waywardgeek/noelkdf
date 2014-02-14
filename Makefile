CFLAGS=-O3 -std=c11 -W -Wall
#CFLAGS=-O3 -std=c11 -W -Wall -msse4.2
#CFLAGS=-g -std=c11 -W -Wall

all: noelkdf-ref noelkdf noelkdf-test fasthash

fasthash: fasthash.c
	gcc $(CFLAGS) -msse4.2 fasthash.c -o fasthash

noelkdf-ref: main.c noelkdf-ref.c noelkdf-common.c noelkdf.h sha256.c sha256.h
	gcc $(CFLAGS) main.c noelkdf-ref.c noelkdf-common.c sha256.c -o noelkdf-ref

noelkdf: main.c noelkdf-pthread.c noelkdf-common.c noelkdf.h sha256.c sha256.h
	gcc $(CFLAGS) -pthread main.c noelkdf-pthread.c noelkdf-common.c sha256.c -o noelkdf
	#gcc -mavx -g -O3 -S -std=c99 -m64 main.c noelkdf-pthread.c noelkdf-common.c sha256.c

noelkdf-test: noelkdf-test.c noelkdf.h noelkdf-ref.c noelkdf-common.c
	gcc $(CFLAGS) noelkdf-test.c noelkdf-ref.c noelkdf-common.c sha256.c -o noelkdf-test

clean:
	rm -f noelkdf-ref noelkdf noelkdf-test
