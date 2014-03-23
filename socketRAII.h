#ifndef socketRAII_h //includeguard
#define socketRAII_h

#include<vector>
#include <netinet/in.h>
#include <string>

	template<typename T, typename alloc>
	std::ostream& operator<<(std::ostream& out, std::vector<T, alloc> v) {
	  for (typename std::vector<T, alloc>::iterator it = v.begin(); it != v.end(); it++)
	    out << *it << ' ';
	  return out;
	}
	
std::ostream& operator<<(std::ostream& out, struct sockaddr_in addr);
std::ostream& operator<<(std::ostream& out, struct message message);
	
	
struct message{
	
	std::vector<char> buffer; //Speicher f�r die gesendete oder empfangene Nachricht
	struct sockaddr_in add; //Adresse f�r den Sender oder vom Empf�nger
	socklen_t addlen; //Die L�nge der Adresse
	
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
		