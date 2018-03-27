CC:=gcc
CFLAGS:=$(CFLAGS) -I../ -D_FILE_OFFSET_BITS=64 -DOS_LINUX

all: findex ffind

findex: findex.o magic.o path.o fs.o db.o hash.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

ffind: ffind.o format.o path.o fs.o db.o hash.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

finfo: finfo.o magic.o path.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -f *.o
	rm -f findex ffind
