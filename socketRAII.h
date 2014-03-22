#ifndef socketRAII_h //includeguard
#define socketRAII_h

#include<vector>
#include <netinet/in.h>
#include <string>

struct message{
	
	std::vector<char> buffer; //Speicher für die gesendete oder empfangene Nachricht
	struct sockaddr_in add; //Adresse für den Sender oder vom Empfänger
	socklen_t addlen; //Die Länge der Adresse
	
	message(size_t size);
	message(size_t size, struct sockaddr_in add, socklen_t addlen);
	message(std::string payload, struct sockaddr_in add, socklen_t addlen);
};

class socketRAII
{
	public:
		int sock;
	
		socketRAII(sockaddr_in sa); //Konstruktor
		socketRAII(int port, long unsigned int ip);
		~socketRAII(); //Destruktor
	
		struct message receive();
		void send(struct message sendemessage);
};
#endif
		