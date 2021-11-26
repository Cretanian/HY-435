server:
	clear
	g++ -std=c++14 -pthread main.cpp
	./a.out -s -p 4001

client:
	clear
	g++ -std=c++14 -pthread main.cpp
	./a.out -c -b 20Mb -t 30 -p 4001 -a "192.168.4.51"

compile:
	clear
	g++ -std=c++14 -pthread main.cpp

oneway:
	clear
	g++ -std=c++14 main.cpp
	./a.out -c -d

clean:
	rm *.out
