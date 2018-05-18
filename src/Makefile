CC:=gcc
CFLAGS:=$(CFLAGS) -I../ -D_FILE_OFFSET_BITS=64 -DOS_LINUX

all: findex ffind ffile

findex: findex.o magic.o path.o fs.o db.o hash.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

ffind: ffind.o format.o magic.o path.o fs.o db.o hash.o array_string.o details.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

ffile: ffile.o magic.o path.o fs.o db.o hash.o array_string.o details.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -f *.o
	rm -f findex ffind ffile
