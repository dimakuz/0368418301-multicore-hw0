all: parsort

parsort: parsort.cpp
	g++ -Wall -g3 -pthread parsort.cpp -o parsort -std=c++11

check: parsort
	../test2.sh ./parsort

clean:
	rm -f parsort

.PHONY: clean
