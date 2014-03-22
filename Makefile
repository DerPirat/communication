all: server
clean:
	rm -f server client *.o

server: server.cpp socketRAII.cpp 
	g++ -g -std=c++0x -pthread -o server server.cpp socketRAII.cpp


client: client.cpp socketRAII.cpp
	g++ -g -std=c++0x -pthread -o client client.cpp socketRAII.cpp