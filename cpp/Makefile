all: parsort

parsort: parsort.cpp
	g++ -Wall -g3 -lpthread parsort.cpp -o parsort

check: parsort
	../test2.sh ./parsort

clean:
	rm -f parsort

.PHONY: clean
