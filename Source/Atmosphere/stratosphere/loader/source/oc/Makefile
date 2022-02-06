export CC := g++-11

all: test

test:
	$(CC) ldr_oc_suite.cpp test.cpp -o ./test -O2 -std=c++20 -DOC_TEST

clean:
	rm ./test
