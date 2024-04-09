all:
	@mkdir bin -p
	g++ --std=c++11 -o bin/decoder src/decoder.cpp

clean:
	rm -f bin/encoder bin/decoder