#include "simple_shell.h"


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





int main(int argc, char * argv[]){
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        std::cerr<<"Fail to create a socket"<<std::endl;
        exit(0);
    }
    int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");
    short port = atoi(argv[1]);


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
    int maxfd = sockfd;
    int clientfd;

    shell NPshell;
    while(1){

        struct sockaddr_in client_info;
        socklen_t len = sizeof(client_info);
        if((clientfd = accept(sockfd, (struct sockaddr * )&client_info, &len ))<0){
            std::cerr<<"Fail to connect"<<std::endl;
            exit(0);
        }


        int pid;
        while((pid=fork()) < 0){
            usleep(1000);
        }

        if(pid == 0){
            close(sockfd);
            client user1;
            user1.id = clientfd - sockfd;
            user1.fd = clientfd;
            strcpy(user1.ip, inet_ntoa(client_info.sin_addr));
            user1.port = ntohs(client_info.sin_port);
            std::cout<<"User "<<user1.nickname<<" entered from "<<user1.ip<<"/"<<user1.port<<std::endl;
            dup2(clientfd, STDIN_FILENO);
            dup2(clientfd, STDOUT_FILENO);
            dup2(clientfd, STDERR_FILENO);
            std::cout<<"% "<<std::flush;
            NPshell.NPloop(clientfd);
            close(clientfd);
            exit(0);
        }
        else{

            int status;
            waitpid(pid,&status, 0);
            close(clientfd);

        }
            

    }
    close(sockfd);
    return 0;
}