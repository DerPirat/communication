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
#include <sstream>
#include <unistd.h>
#include "kennung.h"
     
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


int getch(){
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

class Modus{
	private:
		modstate currentstate;
	public:
		Modus(){
			currentstate = INAKTIV;
		}

		static void * transit(void* arg)
		{
			Modus* test = reinterpret_cast<Modus*>(arg);
			if (test == NULL)
				return NULL;
			
			sleep(3);
			while(test->currentstate == AKTIVIEREND)
				test->currentstate = AKTIV;
			while(test->currentstate == DEAKTIVIEREND)
				test->currentstate = INAKTIV;
			return NULL;
		}
		
		
		void SetState(modstate staterequest){
			
			/*
			std::cout << std::endl <<  "staterequest: " << staterequest << std::endl;
			std::cout << GetState() << std::endl;
			*/
			if ( GetState() == staterequest){
				return;
			}
		
			if ( GetState() == AKTIV && staterequest == INAKTIV){
				currentstate = DEAKTIVIEREND;
			}
			
			if( GetState() == INAKTIV && staterequest == AKTIV){
				currentstate = AKTIVIEREND;
			}
			
			pthread_t transitptr;
				
			pthread_create(&transitptr, NULL, transit, this);
		}
		
		modstate GetState() {
			return currentstate;
		}
};



class Modusmanager{
	
	public:
	
	std::vector<Modus> moduslist;
	Modusmanager(int N) : moduslist(N){};
		
	modstate GetState(char idx) {
		return moduslist[idx].GetState();
	}


	void SetState(char idx, char state) {
		moduslist[idx].SetState(static_cast<modstate>(state));
	}
};

Modusmanager modusmanager(2); //mit 2 Modi initialisieren

std::string makePayload(const char senderID, const char functionID, std::string payload){
	return std::string(1,kennung1) + std::string(1,kennung2) + std::string(1,senderID) + std::string(1,functionID) + payload;
}

//compare ist f�r die clientlist, um zu sehen, ob ein Client bereits in der Menge verbundener Clients enthalten ist
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

//Menge der verbundenen Clients
std::set<sockaddr_in, compare> clientlist;

//server Socket initialisieren auf Port 2001
socketRAII server(2001, INADDR_ANY);


void * modussenden(void *message)
{
	for(;;)
	{
		// Jeden Modus anhand der ID durchlaufen, dessen Status mit getmodus holen und mit senden rausschicken
		for( int idx = 0 ; idx < modusmanager.moduslist.size(); idx++){
			//An jeden Client aus der clientlist nachricht schicken
			for(std::set<sockaddr_in, compare>::iterator it = clientlist.begin(); it != clientlist.end(); it++)
			{
				struct message sendemessage(6, *it, sizeof *it);
				sendemessage.buffer[0] = kennung1;
				sendemessage.buffer[1] = kennung2;
				sendemessage.buffer[2] = senderID_server;
				sendemessage.buffer[3] = functionID_modus;
				sendemessage.buffer[4] = idx;
				sendemessage.buffer[5] = modusmanager.GetState(idx);
				server.send(sendemessage);
			}		
		usleep(20000); //200 ms schlafen legen
		}
	}
	return NULL;
}

int batt = 50;
std::string s1("Batterie");

void *infosenden(void *message){

	for(;;)
	{ 
		if(kbhit())   //kbhit liefert bool ob der Nutzer Taste gedrueckt hat
		{
			char c = getchar(); //c enthaelt den char der gedrueckten Taste

			if(c == '+'){
				batt++;
			}
			if(c == '-'){
				batt--;
			}
			if(c == '+' || c == '-'){
				
				//Umweg �ber sstream, da  sonst ein cast Fehler von int zu const char auftaucht
				std::stringstream ss;
				ss << batt;
				
				std::string payload = makePayload(senderID_server, functionID_info, s1+ '\0' + ss.str() + '\0');

				for(	std::set<sockaddr_in, compare>::iterator it= clientlist.begin(); it != clientlist.end(); it++){
					struct message infomessage(payload, *it, static_cast<socklen_t>(sizeof *it));
					server.send(infomessage);
				}
				
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
		clientlist.insert(rcvmessage.add);
		if(rcvmessage.buffer[0] == kennung1 && rcvmessage.buffer[1] == kennung2 && rcvmessage.buffer[3] == functionID_request){
			char idx = rcvmessage.buffer[4];
			char state = rcvmessage.buffer[5];
			modusmanager.SetState(idx,state);
			std::cout <<"\nNachricht empfangen" << std::endl;
			std::cout << rcvmessage;
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
		
		usleep(3000);
		
		std::cout << '\xd' << "Modus1: " << modusmanager.moduslist[0].GetState() << "   ";
		std::cout <<  "Modus 2: " << modusmanager.moduslist[1].GetState() << "   ";
		std::cout << "Batterie: " << batt;
		
		/*
		std::cout << "Modus1: " << modusmanager.moduslist[0].GetState() << std::endl;
		std::cout <<  "Modus 2: " << modusmanager.moduslist[1].GetState() << std::endl;
		std::cout << "Batterie: " << batt << " %" << std::endl;
		*/
	}
}