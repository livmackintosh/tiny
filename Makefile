.PHONY: clean
DEBUG=false
CFLAGS := -fno-plt -O1 -std=gnu11 -nostartfiles -Wall -Wextra
CFLAGS += -fno-stack-protector -fno-stack-check -fno-unwind-tables -fno-asynchronous-unwind-tables -fomit-frame-pointer
CFLAGS += -no-pie -fno-pic -fno-PIE -fno-PIC -march=core2 -ffunction-sections -fdata-sections

OBJS = track.o device.o tiny.o
LIBS = `pkg-config --cflags gtk+-3.0` -lGL -lgtk-3 -lgdk-3 -lgobject-2.0

all: tiny.elf tiny

clean:
	rm -f src/shader.h tiny.elf tiny $(OBJS)

src/shader.h: src/shader.frag Makefile
	mono ./bin/shader_minifier.exe --preserve-externals src/shader.frag -o ./src/shader.h

%.o: src/%.c
	gcc -c $< $(LIBS) $(CFLAGS)

tiny.elf: src/shader.h $(OBJS)
	gcc -o $@ $< $(LIBS) $(CFLAGS) $(OBJS)

tiny: tiny.elf
	strip -o $@ $< && upx --best  $@
