export CC := g++

all: test

test:
	$(CC) ldr_oc_suite.cpp test.cpp -o ./test -O2 -std=c++20 -Wall

clean:
	rm ./test
