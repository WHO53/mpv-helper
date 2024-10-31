NAME=mpv-helper
BIN=${NAME}
SRC=.

PKGS = locus

MPV_HELPER_SOURCES += $(wildcard $(SRC)/*.c) $(wildcard $(SRC)/**/*.c) $(wildcard $(SRC)/***/**/*.c) 
MPV_HELPER_HEADERS += $(wildcard $(SRC)/*.h) $(wildcard $(SRC)/**/*.h) $(wildcard $(SRC)/***/**/*.h)

CFLAGS += -std=gnu99 -Wall -g
CFLAGS += $(shell pkg-config --cflags $(PKGS))
LDFLAGS += $(shell pkg-config --libs $(PKGS)) -lm -lutil -lrt

SOURCES = $(MPV_HELPER_SOURCES)

OBJECTS = $(SOURCES:.c=.o)

all: ${BIN}

$(OBJECTS): $(MPV_HELPER_HEADERS)

$(BIN):$(OBJECTS)
	$(CC) -o $(BIN) $(OBJECTS) $(LDFLAGS)

install: ${BIN}
	install -m 755 $(BIN) /usr/bin/
	install -D mpv-helper.desktop /usr/share/applications/

uninstall: $(BIN)
	rm -rf /usr/bin/$(BIN)

clean:
	rm -f $(OBJECTS) ${BIN}

format:
	clang-format -i $(MPV_HELPER_SOURCES) $(MPV_HELPER_HEADERS)
