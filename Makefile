all: build
	ninja -C build

run: all
	./build/app

build:
	cmake -B build -G Ninja

clean:
	ninja clean -C build

.PHONY: all run clean
