server:
	clear
	g++ -std=c++11 main.cpp
	./a.out -s

client:
	clear
	g++ -std=c++11 main.cpp
	./a.out -c

clean:
	rm *.out