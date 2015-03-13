CFLAGS:=$(CFLAGS) -I../

all: findex ffind

findex: findex.o magic.o path.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

ffind: ffind.o path.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -f *.o
	rm -f findex ffind
