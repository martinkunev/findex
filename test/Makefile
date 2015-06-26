CFLAGS:=$(CFLAGS) -O2 -I../src/
LDFLAGS:=$(LDFLAGS) -lcmocka -Wl,--wrap=getcwd,--wrap=free

check: check.o ../src/path.o
	$(CC) $^ $(LDFLAGS) -o $@
	./check

clean:
	rm -f *.o
	rm -f check
