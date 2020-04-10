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


int main(int argc, char * argv[]){
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        std::cerr<<"Fail to create a socket"<<std::endl;
        exit(0);
    }

    struct sockaddr_in info;
    bzero(&info, sizeof(info));
    info.sin_family = PF_INET;
    info.sin_addr.s_addr = inet_addr(argv[1]);
    info.sin_port = htons(atoi(argv[2]));

    if(connect(sockfd, (struct sockaddr *)&info, sizeof(info))<0){
        std::cerr<<"Fail to connect"<<std::endl;
        exit(0);
    }

    fd_set rfds, mas;
    FD_ZERO(&rfds);
    FD_ZERO(&mas);
    FD_SET(0, &mas);
    FD_SET(sockfd, &mas);
    char buff[1024];
    while(1){
        rfds = mas;
        if(select(sockfd+1, &rfds, NULL, NULL, NULL)<0){
            std::cerr<<"Fail to select"<<std::endl;
            exit(0);
        }
        for(int i=0;i<=sockfd;i++){
            if(FD_ISSET(i, &rfds)){
                bzero(buff, sizeof(buff));
                if(i==0){
                    fgets(buff, 1024, stdin);
                    if(strncmp(buff, "exit", 4)==0){
                        FD_CLR(0, &rfds);
                        exit(0);
                    }
                    if(send(sockfd, buff, strlen(buff), 0) < 0){
                        std::cerr<<"Fail to send"<<std::endl;
                        exit(0);
                    }
                }
                else {
                    if(recv(sockfd, buff, 1024, 0)<0){
                        std::cerr<<"Fail to recv"<<std::endl;
                        exit(0);
                    }
                    std::cout<<buff;
                }
            }
        }
    }
    close(sockfd);
    return 0;
}