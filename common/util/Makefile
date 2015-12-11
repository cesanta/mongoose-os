HEADERS=error_codes.h logging.h status.h statusor.h
SOURCES=error_codes.cc logging.cc status.cc

all: test-status test-statusor


test-status: ${HEADERS} ${SOURCES}
	c++ -std=c++11 -Wall -Werror -I../.. -o status_test status_test.cc ${SOURCES} -lgtest -lgtest_main -lpthread && ./status_test

test-statusor: ${HEADERS} ${SOURCES}
	c++ -std=c++11 -Wall -Werror -I../.. -o statusor_test statusor_test.cc ${SOURCES} -lgtest -lgtest_main -lpthread && ./statusor_test
