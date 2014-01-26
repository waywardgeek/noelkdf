pebble: pedatabase.c pedatabase.h main.c
	gcc -Wall -O2 -DDD_DEBUG pedatabase.c main.c -o pebble -lm -lddutil-dbg

pedatabase.h: pedatabase.c

pedatabase.c: Pebble.dd
	datadraw Pebble.dd
