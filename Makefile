.PHONY: build clean install log

build:
	pebble build

clean:
	pebble clean

install:
	pebble install --phone $(PHONE)

log:
	pebble logs --phone $(PHONE)
