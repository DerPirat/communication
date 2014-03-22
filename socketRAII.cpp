#include "socketRAII.h"
#include<iostream>
#include <sys/socket.h>
#include <unistd.h> /* for close() for socket */ 
#include <cstring>
//Konstruktoren für struct message
message::message(size_t size): buffer(size,0), add{0}, addlen(0){}
message::message(size_t size, struct sockaddr_in add, socklen_t addlen) : buffer(size,0), add(add), addlen(addlen){}
message::message(std::string payload, struct sockaddr_in add, socklen_t addlen) : buffer(payload.begin(), payload.end()), add(add) ,addlen(addlen){}; 
	//sizeof add könnte krachen gehen, falls das passiert, dann wieder socklen_t addlen nehmen
	//buffer(payload.begin(), payload.end()) kopiert den std::string in den buffer rein
	

socketRAII::socketRAII(sockaddr_in sa){
	sa.sin_family = AF_INET;

	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP); //das soll von Haus aus UDP	
	int e = bind(sock,(struct sockaddr *)&sa, sizeof(sa));
	if (-1 == e){
		close(sock);
		throw 1;
	}
}

socketRAII::socketRAII(int port, unsigned long int ip){

	//IP-Adresse und Port des Servers
	sockaddr_in sa;
        memset(&sa, 0, sizeof sa);
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(ip);
	sa.sin_port = htons(port);
	

	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP); //das soll von Haus aus UDP	
	int e = bind(sock,(struct sockaddr *)&sa, sizeof(sa));
	if (-1 == e){
		close(sock);
		throw 1;
	}
}

socketRAII::~socketRAII(){
	close(sock);
}

struct message socketRAII::receive(){
	
	struct message rcvmessage(1024);
	ssize_t recsize = recvfrom(sock, &(rcvmessage.buffer[0]), rcvmessage.buffer.size(), 0, (struct sockaddr*)&(rcvmessage.add), &(rcvmessage.addlen));
	rcvmessage.buffer.resize(recsize);
			
	return rcvmessage;
}
		
void socketRAII::send(struct message sendemessage){
		
	int bytes_sent = sendto(sock, &(sendemessage.buffer[0]), sendemessage.buffer.size(), 0, (struct sockaddr*)&(sendemessage.add), sendemessage.addlen);
		
	if (bytes_sent < 0) {
		std::cout <<"Sendeversuch gescheitert" << std::endl;
	}
}