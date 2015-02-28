CFLAGS:=$(CFLAGS) -I../ -DTEST -O0 -g
LDFLAGS:=$(LDFLAGS) -lcheck

check: path
	./path

path: ../path.o
	$(CC) $^ $(LDFLAGS) -o $@

clean:
	rm -f *.o
	rm -f path
