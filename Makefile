SRC=rice.c

CFLAGS=-fPIC -O2 
LDFLAGS=-shared

all:
	clang rice.c $(CFLAGS) $(LDFLAGS) -o librice.so
