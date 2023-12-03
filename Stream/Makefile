CFLAGS = -std=c++17 -O2
LDFLAGS = -lSDL2 -lSDL2_ttf

out: *.cpp #*.h
	g++ *.cpp  $(CFLAGS) `pkg-config -cflags opencv4` `pkg-config -libs opencv4` $(LDFLAGS) -o main

.PHONY: test clean

test: main
	./main

clean:
	rm -f main
