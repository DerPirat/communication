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
const char senderID = 0x01; //Client 1, Server hat 0x00;
const char functionID = 0x01;
 
//template für verschiedene Vektoren 
//Gibt Referenz auf ostream zurück
template<typename T, typename alloc>
std::ostream& operator<<(std::ostream& out, std::vector<T, alloc> v) {
  for (typename std::vector<T, alloc>::iterator it = v.begin(); it != v.end(); it++)
    out << *it << ' ';
  return out;
}

class Modus{
	private:
		modstate currentstate;
	public:
		//modstate GetState();
		//void SetState(modstate state);
	
		Modus(){
			currentstate = INAKTIV;
		}

		void SetState(modstate state){
			if ( GetState() == state){
				return;
			}
		
			if ( GetState() == AKTIV && state == INAKTIV){
				
				currentstate = DEAKTIVIEREND;
				
				//SetTimer(&change, 10000, 0);
				// SetTimer muss noch die Funktion setmodus mit den Argumenten übergeben werden setmodus(idx, soll);
				
				currentstate = INAKTIV;
			}
			
			if( GetState() == INAKTIV && state == AKTIV){
				
				currentstate = AKTIVIEREND;
				
				//SetTimer(&change, 10000, 0);
				// SetTimer muss noch die Funktion setmodus mit den Argumenten übergeben werden setmodus(idx, soll);
				currentstate = AKTIV;
			}
		}
		
		modstate GetState() {
			return currentstate;
		}
};

class Modusmanager{
	
	public:
		//an der Stelle das Vektorfeld reinsetzen, Konstruktor soll N modi initialisieren
		std::vector<Modus> moduslist;
		Modusmanager(int N) : moduslist(N){}; //N ist die Anzahl der Modi, alle default INAKTIV
		
		//SetState(int idx, modstate state); //Modi hinter idx auf den modstate state setzen
		//modstate GetState(int idx);
	/*	
	void SetState(char stateID, char idx) {
		moduslist[idx] = modus.SetState;
	}
	*/
	
	modstate GetState(char idx) {
		return moduslist[idx].GetState();
	}


	void SetState(char idx, char state) {
		if (state == INAKTIV)
			moduslist[idx].SetState(AKTIV);
		else if (state == AKTIV)
			moduslist[idx].SetState(INAKTIV);
	}
};

struct statusmessage{
	struct sockaddr_in add; //Adresse für den Sender oder vom Empfänger
	socklen_t addlen; //Die Länge der Adresse
	Modusmanager modusmanager;
	
	statusmessage(struct sockaddr_in add, socklen_t addlen, Modusmanager modusmanager) : add(add),addlen(addlen),modusmanager(modusmanager){}
};

void *sendethread(void *statusmessage)
{
	struct statusmessage *addrmodus = reinterpret_cast<struct statusmessage *>(statusmessage);
	
	if (addrmodus == NULL){
		return NULL;
	}
	
	struct message sendemessage(6, addrmodus->add,addrmodus->addlen);
	
	sendemessage.buffer[0] = kennung1;
	sendemessage.buffer[1] = kennung2;
	sendemessage.buffer[2] = senderID;
	sendemessage.buffer[3] = functionID;
		

	//Socket erstellen, über den die Stati regelmäßig rausgehen
	sockaddr_in statusaddress;
        memset(&statusaddress, 0, sizeof statusaddress);
	statusaddress.sin_addr.s_addr = htonl(INADDR_ANY);
	statusaddress.sin_port = htons(2002);
	socketRAII sendesocket(statusaddress); //Socket erstellen, über den die Message gesendet wird
	
	for(;;)
	{
		// Jeden Modus anhand der ID durchlaufen, dessen Status mit getmodus holen und mit senden rausschicken
		for( int idx = 0 ; idx < addrmodus->modusmanager.moduslist.size(); idx++){
			
			sendemessage.buffer[4] = idx;
			sendemessage.buffer[5] = addrmodus->modusmanager.GetState(idx);
			sendesocket.send(sendemessage);
		}		
		sleep(2); //200 ms schlafen legen
	return NULL;
	}
}

//void *statusthread(){
//	for(;;){
		
//void *input()

namespace Global{ int batt = 50; }

struct requestmessage{
	struct message request;
	SocketRAII socket; //über den Socket geht die Message raus
	
	requestmessage(struct message request, SocketRAII socket) : request(request), socket(socket){}
};


void *clientrequest(void *requestmessage){
	
	/*
	struct message *addrinfo = reinterpret_cast<struct message *>(message);
	
	if (addrinfo == NULL){
		return NULL;
	}
	
	
	struct message requestmessage(1024, addrinfo->add,addrinfo->addlen);
	*/
	
	requestmessage.request.buffer[0] = kennung1;
	requestmessage.request.buffer[1] = kennung2;
	requestmessage.request.buffer[2] = senderID;
	requestmessage.request.buffer[3] = functionID;

	//Socket erstellen, über den der request rausgeht. Finde icht nicht schön, dass soll über den remotesocket laufen.
	/*
	sockaddr_in requestaddress;
        memset(&requestaddress, 0, sizeof requestaddress);
	requestaddress.sin_addr.s_addr = htonl(INADDR_ANY);
	requestaddress.sin_port = htons(2003);
	socketRAII requestsocket(requestaddress);
	*/
	
	// Die Schleife wartet darauf, dass der User eine Eingabe macht. Bei Eingabe wird gefragt ob 1 oder 2 gedrückt wurde, damit wird der Modus gewählt
	//Anschließend wird auf die zweite Eingabe gewartet. Wenn 0 oder 1 gedrückt wurde, wird an den Modus entsprechend c1 die Statusanforderung entsprechend c2 geschickt.
	for(;;)
	{
		if(kbhit()) // if the user presses a key
		{
			char c1 = getchar();
			
			//1 ist 49
			//0 ist 48
			
			if(c1 == 49 || c1 == 50)
			{
				int modreq = c1 - '0';
				modreq = modreq - 48; //jetzt steht in modreq 1 aufwärts
				
				if(kbhit())  
				{
					char c2 = getchar();
					if(c2 == 48 || c2 == 49){
						int statreq = c2 -'0';
						statreq = c2 - 48;
						
						requestmessage.request.buffer[4] = modreq;
						requestmessage.request.buffer[5] = statreq;
						requestmessage.socket.send(requestmessage.request);
					}
				}
			}
			if(c == 3){
				break;
			}
		}
	}
}
	

 

int main(void){
	
	Modusmanager modusmanager(2); //mit 2 Modi initialisieren

	pthread_t sendptr, rcvptr, battptr;
	//pthread_create (&sendptr, NULL, socketRAII::senden, &ch1);
	
	//IP-Adresse und Port des Servers
	sockaddr_in serveraddress;
        memset(&serveraddress, 0, sizeof serveraddress);
	serveraddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddress.sin_port = htons(2001);

        //IP-Addresse und Port des Clients
	sockaddr_in remoteaddress;
        memset(&remoteaddress, 0, sizeof remoteaddress);
        remoteaddress.sin_addr.s_addr = htonl(INADDR_ANY); //Ziel ist eigentlich der Emulator!!
        remoteaddress.sin_port = htons(3001); //Port vom Ziel

        //Socket erstellen, über den requests gesendet werden
        socketRAII s(remoteaddress);

        //Empfangsnachricht festlegen
        struct message rcvmessage(6,serveraddress,sizeof serveraddress);
	
	//Sendenachricht
	//struct statusmessage sendemessage( remoteaddress, sizeof remoteaddress, modusmanager); 
	
	//requestmessage
	struct message requestmessage(1024,serveraddress,sizeof serveraddress);
	
	
	pthread_create(&sendptr, NULL, sendethread, &sendemessage);
	
	void* voidptr;
	pthread_create(&battptr, NULL, infoinput, &infomessage);

	std::cout << "Client..." << std::endl;	

	for (;;) {
		
		//std::cout << "blablabla" << '\xd' << "blub";
		//std::cout << "blablabla\xd" << "blub";
		std::cout << "Client..." << std::endl;
		std::cout << '\xd' << "Modus1: " << modusmanager.moduslist[0].GetState() << "   ";
		std::cout <<  "Modus 2: " << modusmanager.moduslist[1].GetState() << "   ";
		std::cout << "Batterie: " << Global::batt;
		
		
		/*
		
		printf ("Server....\n");
		  
		struct message message = s.receive(rcvmessage);
		  
		std::cout << "recsize: " << message.buffer.size() << std::endl;
		sleep(1);
		 printf("datagram: %.*s\n", (int)message.buffer.size(), &(message.buffer[0]));

		std::cout << "datagram: " <<  message.buffer << std::endl;
	    
		if(message.buffer[0] == kennung1 && message.buffer[1] == kennung2){
			char idx = message.buffer[4];
			char state = message.buffer[5];
			modusmanager.SetState(idx,state);
		}
		*/
		

	}
}