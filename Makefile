server:
	clear
	g++ -std=c++14 -pthread main.cpp
	./a.out -s -f ela.json

client:
	clear
	g++ -std=c++14 -pthread main.cpp
	./a.out -c -b 20Mb -n 2 -t 10

compile:
	clear
	g++ -std=c++14 -pthread main.cpp

oneway:
	clear
	g++ -std=c++14 main.cpp
	./a.out -c -d

clean:
	rm *.out
