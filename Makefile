server:
	clear
	g++ -std=c++11 main.cpp
	./a.out -s -f ela.json

client:
	clear
	g++ -std=c++11 main.cpp
	./a.out -c -b 500Mb

clean:
	rm *.out