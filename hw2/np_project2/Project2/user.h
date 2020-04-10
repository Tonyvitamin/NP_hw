#ifndef USER_OP_HEADER
#define USER_OP_HEADER
#include <string.h>
#include <iostream>
#include <strings.h>
#include <string>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <vector>
#include <cstring>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <signal.h>
#include <netdb.h>
#include <arpa/inet.h>


// broadcast, tell, yell
struct message{
    char  msg[1024] ;
    message(){
        strcpy(msg,"");
    }
    message(std::string words){
        strcpy(msg, words.c_str());
    }
};



// who, 
struct client{
    int id;
    char ip[16];
    pid_t pid;
    int port;
    char nickname[1024];
    client(){
        pid = -1;
        id = -1;
        strcpy(nickname,"(no name)");
        port = -1;
        bzero(ip, sizeof(ip));

    }
    client(int idn, pid_t pidn, std::string name, int portn, char* ipaddr){
        pid = pidn;
        id = idn;
        strcpy(nickname, name.c_str());
        port = portn;
        strcpy(ip, ipaddr);
    }
};
void INTHandler(int signo);

int find_user_bypid(pid_t pid);
struct client * get_client_list(key_t key);
struct message * get_message(key_t key);
void create_clientMem(key_t key);
void create_msgMem(key_t key);

pid_t find_pid_byid(int id);
void detach_client_mem(client * users);
void detach_msg_mem(message * data);


#endif
