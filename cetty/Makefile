
all: cetty

cetty: main.c debug.c debug.h
	gcc -g -DUSE_SYSLOG -Wall main.c debug.c -lezV24 -o cetty

clean:
	rm -f core *.o cetty
