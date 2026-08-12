.PHONY: build clean install log compile

build:
	pebble build

clean:
	pebble clean

install:
	pebble install --phone $(PHONE)

log:
	pebble logs --phone $(PHONE)

compile:
	pebble clean && pebble build && pebble compile-commands
