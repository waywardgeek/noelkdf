CFLAGS=-O3 -std=c11 -W -Wall -funroll-loops
#CFLAGS=-O3 -std=c11 -W -Wall -msse4.2
#CFLAGS=-g -std=c11 -W -Wall

all: tigerkdf-ref tigerkdf tigerkdf-test fasthash parahash

parahash: parahash.c
	gcc -O3 -std=c11 -pthread -msse4.2 parahash.c -o parahash

fasthash: fasthash.c
	gcc $(CFLAGS) -msse4.2 fasthash.c sha256.c -o fasthash

tigerkdf-ref: main.c tigerkdf-ref.c tigerkdf-common.c tigerkdf.h sha256.c sha256.h
	gcc $(CFLAGS) main.c tigerkdf-ref.c tigerkdf-common.c sha256.c -o tigerkdf-ref

tigerkdf: main.c tigerkdf-sse.c tigerkdf-common.c tigerkdf.h sha256.c sha256.h
	gcc $(CFLAGS) -pthread main.c tigerkdf-sse.c tigerkdf-common.c sha256.c -o tigerkdf
	#gcc -mavx -g -O3 -S -std=c99 -m64 main.c tigerkdf-sse.c tigerkdf-common.c sha256.c

tigerkdf-test: tigerkdf-test.c tigerkdf.h tigerkdf-ref.c tigerkdf-common.c
	gcc $(CFLAGS) tigerkdf-test.c tigerkdf-ref.c tigerkdf-common.c sha256.c -o tigerkdf-test

clean:
	rm -f tigerkdf-ref tigerkdf tigerkdf-test
