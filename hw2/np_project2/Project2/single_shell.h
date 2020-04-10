#ifndef SINGLE_SHELL_HEADER
#define SINGLE_SHELL_HEADER

#include <netinet/in.h>
#include <errno.h>
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
#include <regex>

#define CMD_len_MAX_BUFF 256
#define LINE_len_MAX_BUFF 15000
#define PIPE_NUM_MAX 1000 


void childHandler(int signo);


class environ_set{
    public:
        std::vector<std::string> env;
        std::vector<std::string> value;
};

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

class fd_info{
    public:
        int input_fd;
        int output_fd;
        int error_fd;

        fd_info(){
            input_fd = STDIN_FILENO;
            output_fd = STDOUT_FILENO;
            error_fd = STDERR_FILENO;
        }
};

class shell{
    public:
        static std::vector< std::deque<fd_info> >fd_table;
        static std::deque<int> pipe_fd0;
        static std::deque<int> pipe_fd1;
        //static std::deque<client> users;
        static client users[32];
        static std::vector <std::vector< std::string > > user_pipe_command;
        static int user_pipe_in[31][31];
        static int user_pipe_out[31][31];
        static environ_set env_table[31];

        // single process server command
        void user_env(int sockfd);
        void init_users();
        void rwg_loop(int port_n);
        void broadcast(std::string words);
        void NPwho(int sockfd);
        void NPname(int sockfd, std::string name);
        void NPyell(int sockfd, std::string msg);
        void NPtell(int sockfd, int target_id, std::string msg);
        int find_user_byfd(int sockfd);
        int find_user_byid(int id);
        void erase_user(int sockfd);


        int  NPloop(int sockfd);  // revised to handle diiferent socket
        pid_t fork_process(std::vector< std::string > command, int sockfd);
        void NPprintenv(std::string input, int sockfd);
        void NPsetenv(std::string env, std::string path, int sockfd);
        void NPexit(int sockfd); // revised to handle the right user
        void create_socket();
        void parse(std::string input, int sockfd);


        shell(){
            // Initiate PATH environment
            //clearenv();
            int check = setenv("PATH", "bin:.", 1);

            if(check==-1){
                write(2, "Setenv error\n", sizeof("Setenv error\n"));
            }
        }
};

#endif
