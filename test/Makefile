CFLAGS:=$(CFLAGS) -I../ -DTEST
LDFLAGS:=$(LDFLAGS) -lcheck

check: path
	./path

path: ../src/path.o
	$(CC) $^ $(LDFLAGS) -o $@

clean:
	rm -f *.o
	rm -f path
