#ifndef MULTI_SHELL_HEADER
#define MULTI_SHELL_HEADER

#include "user.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <iostream>
#include <queue>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <regex>


#define CMD_len_MAX_BUFF 256
#define LINE_len_MAX_BUFF 15000
#define PIPE_NUM_MAX 1000 



static int user_pipe_fd_in[31][31];
static int user_pipe_fd_out[31][31];

void childHandler(int signo);
void User_pipeHandler(int signo);
void MSGHandler(int signo);

int  user_login(key_t client_key, key_t msg_key,  int sockfd, struct sockaddr_in client_info);
void user_logout(key_t key);
void broadcast(std::string words, key_t client_key, key_t msg_key);



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
        static std::deque<fd_info> fd_table;

        static std::deque<int> pipe_fd0;
        static std::deque<int> pipe_fd1;

        int find_user_byfd(int sockfd, key_t key);
        void NPyell(std::string msg, key_t key);
        void NPname(std::string name, key_t key);
        void NPtell(std::string words, int target_id);
        void NPwho();


        void NPloop(key_t key);
        void parse(std::string input);
        pid_t fork_process(std::vector< std::string > command);
        void NPprintenv(std::string input);
        void NPsetenv(std::string env, std::string path);
        void NPexit();


        shell(){
            // Initiate PATH environment
            clearenv();
            int check = setenv("PATH", "bin:.", 1);

            if(check==-1){
                write(2, "Setenv error\n", sizeof("Setenv error\n"));
            }
        }
};

#endif

