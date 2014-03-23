#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <pthread.h>
#include <iostream>
#include "socketRAII.h"
 #include <termios.h>
     int kbhit(void) {

       struct termios term, oterm;

       int fd = 0;

       int c = 0;

       tcgetattr(fd, &oterm);

       memcpy(&term, &oterm, sizeof(term));

       term.c_lflag = term.c_lflag & (!ICANON);

       term.c_cc[VMIN] = 0;

       term.c_cc[VTIME] = 1;

       tcsetattr(fd, TCSANOW, &term);

       c = getchar();

       tcsetattr(fd, TCSANOW, &oterm);

       if (c != -1)

       ungetc(c, stdin);

       return ((c != -1) ? 1 : 0);

    }
    
    int getch()
    {

       static int ch = -1, fd = 0;

       struct termios neu, alt;

       fd = fileno(stdin);

       tcgetattr(fd, &alt);

       neu = alt;

       neu.c_lflag &= ~(ICANON|ECHO);

       tcsetattr(fd, TCSANOW, &neu);

       ch = getchar();

       tcsetattr(fd, TCSANOW, &alt);

       return ch;

    }
    
    
 typedef enum modstate
{
    INAKTIV = 0x00,    
    AKTIV = 0x01,    
    DEAKTIVIEREND = 0x02,
    AKTIVIEREND = 0x03
} modstate;

const char kennung1 = 0x3C;
const char kennung2 = 0x01;
const char senderID_server = 0x00;
const char senderID_client1 = 0x01; //Client 1, Server hat 0x00;
const char functionID_modus = 0x01;
const char functionID_request = 0x02;
const char functionID_info = 0x03;
 
//template für verschiedene Vektoren 
//Gibt Referenz auf ostream zurück
template<typename T, typename alloc>
std::ostream& operator<<(std::ostream& out, std::vector<T, alloc> v) {
  for (typename std::vector<T, alloc>::iterator it = v.begin(); it != v.end(); it++)
    out << *it << ' ';
  return out;
}

//server Client initialisieren auf Port 3001
socketRAII client(3001, INADDR_ANY);


void *clientrequest(void *message){
	
	//IP-Adresse und Port des Servers
	sockaddr_in serveraddress;
        memset(&serveraddress, 0, sizeof serveraddress);
	serveraddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddress.sin_port = htons(2001);
	
	struct message requestmessage(6, serveraddress, sizeof serveraddress);
	requestmessage.buffer[0] = kennung1;
	requestmessage.buffer[1] = kennung2;
	requestmessage.buffer[2] = senderID_client1;
	requestmessage.buffer[3] = functionID_request;

	// Die Schleife wartet darauf, dass der User eine Eingabe macht. Bei Eingabe wird gefragt ob 1 oder 2 gedrückt wurde, damit wird der Modus gewählt
	//Anschließend wird auf die zweite Eingabe gewartet. Wenn 0 oder 1 gedrückt wurde, wird an den Modus entsprechend c1 die Statusanforderung entsprechend c2 geschickt.
	for(;;)
	{
		//if(kbhit()) // if the user presses a key
		//{
			//char c1 = getchar();
			char c1;
			char c2;
			std::cout << "Modusrequest:" << std::endl;
			std::cin >> c1;
			
			if(c1 == '0' || c1 == '1')
			{
				std::cout << "Statusrequest:" << std::endl;
				std::cin >> c2;
				int modrequest = c1 - '0';				
				//if(kbhit())  
				//{
					//char c2 = getchar();
					
					//std::cout << "c2:" << c2 << std::endl;
					
					if(c2 == '0' || c2 == '1'){
						int staterequest = c2 -'0';
						
						requestmessage.buffer[4] = modrequest;
						requestmessage.buffer[5] = staterequest;
						client.send(requestmessage);
						std::cout << "gesendet" << std::endl;
					}
				//}
			}
			//strg +c
			if(c1 == 3){
				break;
			}
		//}
	}
}


void *empfangen(void *message){

	for(;;){
		struct message rcvmessage = client.receive();
		
		if(rcvmessage.buffer[0] == kennung1 && rcvmessage.buffer[1] == kennung2 && rcvmessage.buffer[2] == senderID_server){

		}
	}
}

 

int main(void){
	
	pthread_t requestptr, receiveptr;
	
	pthread_create(&requestptr, NULL, clientrequest, NULL);
	
	pthread_create(&receiveptr, NULL, empfangen, NULL);

	std::cout << "Client..." << std::endl;
	
	for(;;){
	}
	/*
	std::cout << "Verbinde..." << std::endl;
	
	//IP-Adresse und Port des Servers
	sockaddr_in serveraddress;
        memset(&serveraddress, 0, sizeof serveraddress);
	serveraddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddress.sin_port = htons(2001);
	
	struct message verbindungsmessage(6, serveraddress, sizeof serveraddress);
	
	client.send(verbindungsmessage);
	
	struct message rcvmessage = client.receive();
	
	if(rcvmessage.buffer[0] == kennung1 && rcvmessage.buffer[1]  == kennung2){
		std::cout << "Verbunden!" << std::endl;
	}
	*/
	


	


}