all:
	
	g++  -std=c++11 -pthread client.cpp -o client
	g++  -std=c++11 -c randomMessage.cpp -o randomMessage.o
	g++  -std=c++11 -c server.cpp -o server.o
	g++  -std=c++11 ip.cpp -o ip

	g++  -std=c++11 server.o randomMessage.o -o server
	

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

