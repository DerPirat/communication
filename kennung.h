#ifndef kennung_h //includeguard
#define kennung_h

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
const char senderID_client1 = 0x01;
const char functionID_modus = 0x01;
const char functionID_request = 0x02;
const char functionID_info = 0x03;

#endif