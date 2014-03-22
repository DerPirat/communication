#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <pthread.h>
#include <iostream>
#include "socketRAII.h"
#include <termios.h>
#include <set>


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
const char senderID = 0x00;
const char functionID_modus = 0x01;
const char functionID_request = 0x02;
const char functionID_info = 0x03;
 
//template fürsockaddr_in remoteaddress; verschiedene Vektoren 
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

Modusmanager modusmanager(2); //mit 2 Modi initialisieren

struct statusmessage{
	struct sockaddr_in add; //Adresse für den Sender oder vom Empfänger
	socklen_t addlen; //Die Länge der Adresse
	Modusmanager modusmanager;
	
	statusmessage(struct sockaddr_in add, socklen_t addlen, Modusmanager modusmanager) : add(add),addlen(addlen),modusmanager(modusmanager){}
};

std::string makePayload(const char senderID, const char functionID, std::string payload){
	return std::string(1,kennung1) + std::string(1,kennung2) + std::string(1,senderID) + std::string(1,functionID) + payload + '\0';
}

struct compare {
	bool operator()(const sockaddr_in& lhs, const sockaddr_in& rhs) {
		if (lhs.sin_family < rhs.sin_family)
			return true;
		if (lhs.sin_family > rhs.sin_family)
			return false;
		// sin_family is equal
		if (lhs.sin_addr.s_addr < rhs.sin_addr.s_addr)
			return true;
		if (lhs.sin_addr.s_addr > rhs.sin_addr.s_addr)
			return false;
		// s_addr is equal
		if (lhs.sin_port < rhs.sin_port)
			return true;
		return false;
	}
};

std::set<sockaddr_in, compare> clientlist; //Liste für die verbundenen Clients	

socketRAII server(2001, INADDR_ANY);


void * modussenden(void *message)
{
	struct message *addrmodus = reinterpret_cast<struct message *>(message);
	
	if (addrmodus == NULL){
		return NULL;
	}
	
	for(;;)
	{
		// Jeden Modus anhand der ID durchlaufen, dessen Status mit getmodus holen und mit senden rausschicken
		for( int idx = 0 ; idx < modusmanager.moduslist.size(); idx++){
			//An jeden Client aus der clientlist nachricht schicken
			for(::std::set<sockaddr_in, compare>::iterator it = clientlist.begin(); it != set.end(); it++)
			{
				struct message sendemessage(6, it->add, sizeof it->add);
				sendemessage.buffer[0] = kennung1;
				sendemessage.buffer[1] = kennung2;
				sendemessage.buffer[2] = senderID;
				sendemessage.buffer[3] = functionID_modus;
				sendemessage.buffer[4] = idx;
				sendemessage.buffer[5] = modusmanager.GetState(idx);
				server.send(sendemessage);
			}		
		sleep(200); //200 ms schlafen legen
		}
	}
	return NULL;
}

//void *statusthread(){
//	for(;;){
		
//void *input()

int batt = 50;
std::string s1("Batterie");

void *infosenden(void *message){
	
	struct message *addrinfo = reinterpret_cast<struct message *>(message);
	
	if (addrinfo == NULL){
		return NULL;
	}
	
	struct message infomessage(1024, addrinfo->add,addrinfo->addlen);
	
	infomessage.buffer[0] = kennung1;
	infomessage.buffer[1] = kennung2;
	infomessage.buffer[2] = senderID;
	infomessage.buffer[3] = functionID;
	
	for(;;)
	{  // give a second to hit something
		if(kbhit())            // if the user presses a key
		{
			char c = getchar();

			if(c == '+'){
				batt++;
			}
			if(c == '-'){
				batt--;
			}
			if(c == '+' || c == '-'){
				
				std::string payload = makePayload(senderID, functionID_info, s1+ std::string(batt));
				struct message infomessage(payload, remoteaddress);
				
				server.send(infomessage);
			}

			//damit er mir strg+c noch nimmt
			if(c == 3){
				break;
			}
		}
	}
}

void *empfangen(void *message){

	for(;;){
		struct message rcvmessage = server.receive();
		clientlist.insert(rcvmessage.remoteaddress);
		
		if(rcvmessage.buffer[0] == kennung1 && rcvmessage.buffer[1] == kennung2 && rcvmessage == functionID_request){
			char idx = message.buffer[4];
			char state = message.buffer[5];
			modusmanager.SetState(idx,state);
		}
	}
}

int main(void){
	
	pthread_t sendmodptr, sendinfoptr, rcvptr;

	pthread_create(&sendmodptr, NULL, modussenden, NULL);
	
	pthread_create(&sendinfoptr, NULL, empfangen, NULL);
	
	pthread_create(&rcvptr, NULL, infosenden, NULL);

	std::cout << "Server..." << std::endl;
	
	for (;;) {
		
		std::cout << '\xd' << "Modus1: " << modusmanager.moduslist[0].GetState() << "   ";
		std::cout <<  "Modus 2: " << modusmanager.moduslist[1].GetState() << "   ";
		std::cout << "Batterie: " << batt << " %";
		
	}
}