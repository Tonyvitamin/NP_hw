//#include "shell.h"


#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <queue>
#include <string.h>
#include <cstring>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>

class client{

public:
    int id;
    int fd;
    char ip[16];
    int port;
    std::string nickname;
    client(){
        id = -1;
        nickname="(no name)";
        fd = -1;
        port = -1;
        bzero(ip, sizeof(ip));

    }
};

std::vector<int> id_table(30, 0);


int main(int argc, char * argv[]){
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        std::cerr<<"Fail to create a socket"<<std::endl;
        exit(0);
    }

    int port = 7001;


    struct sockaddr_in server_info;
    bzero(&server_info, sizeof(server_info));
    server_info.sin_family = PF_INET;
    server_info.sin_addr.s_addr = INADDR_ANY;
    server_info.sin_port = htons(port);

    bind(sockfd, (struct sockaddr *)&server_info, sizeof(server_info));
    listen(sockfd, 30);

    fd_set rfds, mas;
    FD_ZERO(&rfds);
    FD_ZERO(&mas);
    FD_SET(0, &mas);
    FD_SET(sockfd, &mas);
    char buff[1024];
    std::deque< client > user;
    int maxfd = sockfd;
    while(1){
        rfds = mas;
        if(select(maxfd+1, &rfds, NULL, NULL, NULL)<0){
            std::cerr<<"Fail to select"<<std::endl;
            exit(0);
        }
        for(int i=0;i<=maxfd;i++){
            //std::cout<<i<<std::endl;
            if(FD_ISSET(i, &rfds)){
                bzero(buff, sizeof(buff));
                if(i==0){
                    fgets(buff, 1024, stdin);
                    if(strncmp(buff, "exit", 4)==0){
                        FD_CLR(0, &rfds);
                        exit(0);
                    }
                }
                // connection created
                else if(i==sockfd) {
                    struct sockaddr_in client_info;
                    int clientfd;
                    socklen_t len = sizeof(client_info);
                    if((clientfd = accept(sockfd, (struct sockaddr * )&client_info, &len ))<0){
                        std::cerr<<"Fail to connect"<<std::endl;
                        exit(0);
                    }
                    //std::cout<<"maxfd is "<<maxfd<<" clientfd is "<<clientfd<<std::endl;

                    client user1;
                    user1.id = clientfd - sockfd;
                    user1.fd = clientfd;
                    strcpy(user1.ip, inet_ntoa(client_info.sin_addr));
                    user1.port = ntohs(client_info.sin_port);
                    user.push_back(user1);
                    maxfd = (clientfd > maxfd)?clientfd:maxfd;
                    std::cout<<"User "<<user1.nickname<<" entered from "<<user1.ip<<"/"<<user1.port<<std::endl;
                    FD_SET(clientfd, &mas);
                }
                // other socket action
                else if(i>2){
                    std::string temp;
                    int pos ;
                    //printf("here\n");

                    if(recv(i, &temp, 1024, 0)<=0){
                        std::cout<<"Client leaves, fd: "<< i<<std::endl;
                        for(int j=0;j<user.size();j++){
                            if(user[j].fd == i){
                                pos = j;
                                break;
                            }
                        }
                        temp = "[Server]";
                        temp += user[i].nickname.c_str();
                        temp += " is offline\n" ;
                        for(int j=0;j<user.size();j++){
                            if(j!=pos && user[j].fd >=0){
                                send(user[j].fd, temp.c_str(), strlen(temp.c_str()), 0);
                            }
                        }
                        user[pos].fd = -1;
                        FD_CLR(i, &mas);
                        close(i);
                    }
                    else{
                        // implement yell
                        if(temp[0]=='y'&&temp[1]=='e'&&temp[2]=='l'&&temp[3]=='l'){
                            char buf[1024];
                            for(int j=0;j<user.size();j++){
                                std::cout<<"user "<< user[pos].nickname.c_str()<<"speaking\n";
                                sprintf(buf, "*** %s yelled ***: %s\n", user[pos].nickname.c_str(), temp.substr(4, temp.size()-4));
                                send(user[j].fd, buf, strlen(buf), 0);
                            }
                        }

                        

                    }
                }
            }
        }
    }
    close(sockfd);
    return 0;
}