all: main

.PHONY:
main: window.c window.h kvm.c kvm.h main.c
	cc main.c window.c kvm.c -o main -I/usr/include/SDL2 -D_REENTRANT -lSDL2 -lSDL2_ttf -lpthread
