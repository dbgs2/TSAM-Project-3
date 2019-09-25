all:
	g++  -std=c++11 -pthread client.cpp -o client
	g++  -std=c++11 server.cpp -o server
	g++  -std=c++11 ip.cpp -o ip

client:
	g++  -std=c++11 -pthread client.cpp -o server

server:
	g++  -std=c++11 server.cpp -o server

ip:
	g++  -std=c++11 ip.cpp -o ip

clean:
	rm -rf *o server
	rm -rf *o client
	rm -rf *o ip
