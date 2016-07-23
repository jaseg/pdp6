
CFLAGS := -std=c11 -Wall -Wpedantic `sdl-config --cflags` `pkg-config SDL_image --cflags --libs` -lpthread

all: pdp6 libpdp6.so

pdp6: sdl.c libpdp6.so
	$(CC) $(CFLAGS) -L. -lpdp6 -o $@ $<

libpdp6.so: emu.c apr.c mem.c tty.c
	$(CC) -shared -fPIC $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f pdp6 libpdp6.so

