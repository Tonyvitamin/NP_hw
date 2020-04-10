#ifndef SHELL_HEADER
#define SHELL_HEADER

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

//void NPloop();

//void parse(char *input);

//void NPprintenv(std::string input);

//void NPexit();

//void NPsetenv(std::string env, std::string path);

void childHandler(int signo);

//void fork_process(std::string command);

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
        void NPloop();
        void parse(std::string input);
        pid_t fork_process(std::vector< std::string > command);
        void NPprintenv(std::string input);
        void NPsetenv(std::string env, std::string path);
        void NPexit();


        shell(){
            // Initiate PATH environment
            int check = setenv("PATH", "bin:.", 1);

            if(check==-1){
                write(2, "Setenv error\n", sizeof("Setenv error\n"));
            }
        }
};

#endif
