PROF = -fprofile-arcs -ftest-coverage -g -O0
CFLAGS = -W -Wall -pedantic -Og $(PROF) $(CFLAGS_EXTRA) -fsanitize=address -fsanitize=leak -std=c99
CXXFLAGS = -W -Wall -pedantic -Og $(PROF) $(CFLAGS_EXTRA)

.PHONY: clean all

all: c c++

c: clean
	rm -rf *.gc*
	clang-3.6 unit_test.c -o unit_test $(CFLAGS) && ./unit_test
	# TODO(dfrank): add missing software to ourci docker image and enable -m32 test!
	#clang-3.6 -m32 unit_test.c -o unit_test $(CFLAGS) && ./unit_test
	#gcov -a unit_test.c

c++: clean
	rm -rf *.gc*
	g++ unit_test.c -o unit_test $(CXXFLAGS) && ./unit_test
	gcov -a unit_test.c

w:
	wine cl /DEBUG unit_test.c && wine unit_test.exe

clean:
	rm -rf *.gc* *.dSYM unit_test unit_test.exe
