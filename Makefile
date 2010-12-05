PREFIX =	/usr/local

CFLAGS +=	$(shell sdl-config --cflags) $(shell pkg-config --cflags jack) -W -Wall -std=gnu99
LIBS =		$(shell sdl-config --libs) $(shell pkg-config --libs jack) -lGL -lX11

CFLAGS +=	-O2

OBJS =		main.o video.o audio.o waves.o
BIN =		jack_oscrolloscope


all:		$(BIN)

$(BIN): $(OBJS)
		gcc -o $(BIN) $(OBJS) $(LIBS)

%.o: %.c
		gcc -c -o $@ $(CFLAGS) $*.c

clean:
		rm -f $(BIN) $(OBJS) .dep

install: $(BIN)
	/usr/bin/install -m 755 $(BIN) $(PREFIX)/bin


dep:
	rm -f .dep; \
	for f in *.c; \
	do \
	    gcc -MM $$f $(CFLAGS) >> .dep || exit 1; \
	done

ifeq (.dep,$(wildcard .dep))
include .dep
endif

