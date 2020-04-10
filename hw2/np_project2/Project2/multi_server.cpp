#include "multi_shell.h"
#include "user.h"


int main(int argc, char * argv[]){
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        std::cerr<<"Fail to create a socket"<<std::endl;
        exit(0);
    }

    int port = atoi(argv[1]);

    int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

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


    // set 8787 as the default of memory key 
    create_clientMem(8787);
    create_msgMem(9487);
    std::deque<int> fd_list;



    while(1){
        struct sockaddr_in client_info;
        int clientfd;
        socklen_t len = sizeof(client_info);
        if((clientfd = accept(sockfd, (struct sockaddr * )&client_info, &len ))<0){
            std::cerr<<"Fail to connect"<<std::endl;
            exit(0);
        }
        pid_t pid;
        signal(SIGCHLD, childHandler);
        signal(SIGINT, INTHandler);
        signal(SIGUSR1, MSGHandler);
        signal(SIGUSR2, User_pipeHandler);
        while((pid = fork()) < 0){
            usleep(1000);
        }
        if(pid==0){
            client user1;
            strcpy(user1.ip, inet_ntoa(client_info.sin_addr));
            user1.port = ntohs(client_info.sin_port);
            std::string come_in = "*** User "+ std::string(user1.nickname) + " entered from "+ std::string(user1.ip) + ":"+ std::to_string(user1.port)+ ". ***\n";

            std::cout<<std::flush;
            std::cout<<come_in<<std::endl<<std::flush;
            dup2(clientfd, STDIN_FILENO);
            dup2(clientfd, STDOUT_FILENO);
            dup2(clientfd, STDERR_FILENO);
            close(sockfd);

            user_login(8787, 9487, clientfd, client_info);
            shell npshell;
            npshell.NPloop(8787);
            user_logout(8787);
            shutdown(clientfd, 2);
            close(clientfd);
            exit(0);
            
        }
        else{
            close(clientfd);
        }
    }
   
    close(sockfd);
    return 0;
}
