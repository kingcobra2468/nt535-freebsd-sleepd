CC        = cc
CFLAGS    = --extra-warnings -I/usr/include/dev/acpica/ -I/usr/local/include -I/usr/local/include/linux/
LDFLAGS   = -L/usr/local/lib
LIBS      = -linput -lutil
TARGET    = nt535sleepd
BUILD_DIR = build

$(TARGET): setup nt535sleepd.c
	$(CC) $< -o $(BUILD_DIR)/$@ $(CFLAGS) $(LDFLAGS) $(LIBS)

.PHONY: setup
setup:
	mkdir -p $(BUILD_DIR)

.PHONY: clean
clean:
	rm -fr $(BUILD_DIR)/*
