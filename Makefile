server:
	clear
	g++ -std=c++11 main.cpp
	./a.out -s -f ela.json

client:
	clear
	g++ -std=c++11 main.cpp
	./a.out -c -b 650Mb

clean:
	rm *.out
