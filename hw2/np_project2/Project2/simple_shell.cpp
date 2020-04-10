#include "simple_shell.h"


std::deque<fd_info> shell::fd_table(1500);
std::deque<int>shell::pipe_fd0;
std::deque<int>shell::pipe_fd1;


void shell::NPloop(int sockfd){
    for(;;){
        //char  line[LINE_len_MAX_BUFF];

        // Encounter EOF 
        std::string line;
        if(!std::getline(std::cin, line)){
            shutdown(sockfd,2);
            //close(sockfd);
            return;
        }

        std::string raw(line);
        //for(int i=0;i<raw.size();i++){
        //    std::cout<<raw[i]<<" "<<(int)raw[i]<<std::endl;
        //}
        //raw = raw.substr(0, raw.size()-1);
        std::istringstream separate(raw);
        std::string precision;
        separate>>precision;
        if(raw.size() > 15000){
            fprintf(stderr, "Unable to input too words many in a line\n");
            continue;
        }
        /* built-in command */
        if(precision=="printenv" && raw.find("printenv")!=std::string::npos){
            std::istringstream s(raw);
            std::string path;
            s>>path;
            s>>path;
            NPprintenv(path);

        }
        else if(precision=="setenv" &&  raw.find("setenv")!=std::string::npos){
            std::istringstream s(raw);
            std::string env, value;
            s>>env;
            s>>env;
            s>>value;
            NPsetenv(env, value);
        }
        else if(precision=="exit" && raw.find("exit")!=std::string::npos){
            NPexit(sockfd);
            return;
        }
        /* exec-liked command*/
        else if(!raw.empty() && raw != "\n"){
            parse(raw);
        
        }
        while(fd_table.size() < PIPE_NUM_MAX){
            fd_table.push_back(fd_info());
        }
        std::cout<<"% "<<std::flush;

    }
    return;
}
void shell::parse(std::string input){

    /* strtok to parse the command arguments*/
    //har * cmdtowork[CMD_len_MAX_BUFF];
    std::vector<std::string> cmdtowork;
    char * tmpforinput;
    tmpforinput = new char [1024];
	int cmt=1;
	tmpforinput = strtok((char*)input.c_str() , " \t\n\r");

	cmdtowork.push_back(strdup(tmpforinput));
    while((tmpforinput = strtok(NULL , " \t\n\r"))!=NULL){
		cmdtowork.push_back(strdup(tmpforinput));
		//std::cout<<cmdtowork[cmt]<<std::endl;
		cmt++;

	}
	//std::cout<<cmt<<std::endl;

    int command_group_num = 0; 
    // check parse result and set pipe number and redirection info
    std::vector<std::string> group;
    bool lineEndsWithPipeN = false;
    std::regex reg1("\\|\\d*");
    std::regex reg2("!\\d*");
    for(int i = 0 ;i<cmt;i++){

        std::string tmp(cmdtowork[i]);

        // Case |n, !n, |, ! 
        //if(tmp.find("|")!=std::string::npos || tmp.find("!")!=std::string::npos ){
        if(std::regex_match(tmp, reg1) || std::regex_match(tmp, reg2)){

            // Case |n or !n
            command_group_num++;
            if(tmp.size()>1){
                bool pipe_error = (tmp[0] == '!') ? true : false;
                
                tmp.erase(0,1);
                std::istringstream ss(tmp);
                int pipe_end;
                ss>>pipe_end;
                //std::cout<<"pipe end is "<<pipe_end<<std::endl;

                // If we need to create a pipe
                if(fd_table[pipe_end].input_fd==STDIN_FILENO){
                    int pipe_fd[2];
                    while(pipe(pipe_fd)<0){
                        usleep(1000);
                    }                 
                    //std::cout<<"command group "<<command_group_num<<" |"<<pipe_end<<" create pipe"<<pipe_fd[0]<<" "<<pipe_fd[1]<<std::endl;
                    fd_table[pipe_end].input_fd = pipe_fd[0];
                    
                    fd_table.front().input_fd = (fd_table.front().input_fd!=STDIN_FILENO)?fd_table.front().input_fd: STDIN_FILENO;
                    fd_table.front().output_fd = pipe_fd[1];
                    fd_table.front().error_fd = (pipe_error) ? pipe_fd[1] : STDERR_FILENO;

                    pipe_fd0.push_back(pipe_fd[0]);
                    pipe_fd1.push_back(pipe_fd[1]);
                }

                // using existing pipe    
                else {
                    int write_end_fd = fd_table[pipe_end].input_fd;
                    auto it = std::find(pipe_fd0.begin(), pipe_fd0.end(), write_end_fd);
                    auto idx = std::distance(pipe_fd0.begin(), it);
                    
                    
                    fd_table.front().input_fd = (fd_table.front().input_fd!=STDIN_FILENO)?fd_table.front().input_fd: STDIN_FILENO;
                    fd_table.front().output_fd = pipe_fd1[idx];
                    fd_table.front().error_fd = (pipe_error) ? pipe_fd1[idx] : STDERR_FILENO;
                }


                lineEndsWithPipeN = (i==cmt-1) ? true : false;
            }
            // Case | or !
            else{
                bool pipe_error = (tmp[0] == '!') ? true : false;

                // If we need to create a pipe
                if(fd_table[1].input_fd==STDIN_FILENO){
                    int pipe_fd[2];
                    while(pipe(pipe_fd)<0){
                        usleep(1000);
                    }
                    fd_table[1].input_fd= pipe_fd[0];
                    //std::cout<<"command group "<<command_group_num<<" | create pipe"<<pipe_fd[0]<<" "<<pipe_fd[1]<<std::endl;

                    fd_table.front().input_fd = (fd_table.front().input_fd!=STDIN_FILENO)?fd_table.front().input_fd: STDIN_FILENO;
                    fd_table.front().output_fd = pipe_fd[1];
                    fd_table.front().error_fd = (pipe_error) ? pipe_fd[1] : STDERR_FILENO;

                    pipe_fd0.push_back(pipe_fd[0]);
                    pipe_fd1.push_back(pipe_fd[1]);
                }

                // using existing pipe
                else {
                    int write_end_fd = fd_table[1].input_fd;
                    auto it = std::find(pipe_fd0.begin(), pipe_fd0.end(), write_end_fd);
                    auto idx = std::distance(pipe_fd0.begin(), it);
                    
                    
                    fd_table.front().input_fd = (fd_table.front().input_fd!=STDIN_FILENO)?fd_table.front().input_fd: STDIN_FILENO;
                    fd_table.front().output_fd = pipe_fd1[idx];
                    fd_table.front().error_fd = (pipe_error) ? pipe_fd1[idx] : STDERR_FILENO;                
                }
                


                lineEndsWithPipeN = (i==cmt-1) ? true : false;
            }
            pid_t pid = fork_process(group);
            if(fd_table.front().input_fd != STDIN_FILENO){
                auto it = std::find(pipe_fd0.begin(), pipe_fd0.end(), fd_table.front().input_fd);
                auto idx = std::distance(pipe_fd0.begin(), it);
                //std::cout<<"command group "<<command_group_num<<" close pipe"<<pipe_fd0[idx]<<" "<<pipe_fd1[idx]<<std::endl;

                close(pipe_fd0[idx]);
                close(pipe_fd1[idx]);
                pipe_fd0.erase(pipe_fd0.begin()+idx);
                pipe_fd1.erase(pipe_fd1.begin()+idx);
            }

            fd_table.pop_front();
            fd_table.push_back(fd_info());
            

            group.clear();
        }
        // Last command in the line(except |n, !n, |, !)
        else if(i==cmt-1){
            group.push_back(tmp);
            pid_t pid = fork_process(group);
            if(fd_table.front().input_fd != STDIN_FILENO){
                auto it = std::find(pipe_fd0.begin(), pipe_fd0.end(), fd_table.front().input_fd);
                auto idx = std::distance(pipe_fd0.begin(), it);
                //std::cout<<"command group "<<command_group_num <<" close pipe"<<pipe_fd0[idx]<<" "<<pipe_fd1[idx]<<std::endl;

                close(pipe_fd0[idx]);
                close(pipe_fd1[idx]);
                pipe_fd0.erase(pipe_fd0.begin()+idx);
                pipe_fd1.erase(pipe_fd1.begin()+idx);
            }

            fd_table.pop_front();
            fd_table.push_back(fd_info());
            if(!lineEndsWithPipeN){
                int status;
                waitpid(pid, &status, 0);
                //std::cout<<"waitpid catch child "<< pid<<std::endl;
            }
        }
        else
            group.push_back(tmp);
    }
    //*/
    cmdtowork.clear();
    delete [] tmpforinput;
    return;

}

void shell::NPprintenv(std::string input){
    const char * c_input = input.c_str();
    char * path = getenv(c_input);
    if(path != NULL){
        fprintf(stdout,"%s\n", path);
    }
    fd_table.pop_front();

    return ;
}

void shell::NPexit(int sockfd){
    shutdown(sockfd, 2);
    //close(sockfd);
    return;
}

void shell::NPsetenv(std::string env, std::string path){
    const char * name = env.c_str();
    const char * value = path.c_str();
    int overwrite = 1;

    int check = setenv(name, value, overwrite);
    fd_table.pop_front();

    if(check==-1){
        write(1, "Setenv error\n", sizeof("Setenv error\n"));
    }
    return;
}

void childHandler(int signo){
    int status;
    pid_t pid;
    while(( pid = waitpid(-1, &status, WNOHANG) )> 0){/*std::cout<<"signal catch "<<pid<<std::endl;*/}
    return;
}

pid_t shell::fork_process(std::vector< std::string> command){
    signal(SIGCHLD, childHandler);

    pid_t pid;
    while((pid=fork())<0){
        usleep(1000);
    }
    //std::cout<<fd_table.size()<<" "<<pipe_fd0.size()<<" "<< pipe_fd1.size()<<std::endl;
    // Child process 
    // handle exec like command
    if(pid==0){         
    //std::cout<<getpid()<<" "<<fd_table.front().input_fd<<" "<<fd_table.front().output_fd<<" "<<fd_table.front().error_fd<<std::endl;

        // |n or !n target command
        if(fd_table.front().input_fd != STDIN_FILENO){
            dup2(fd_table.front().input_fd, STDIN_FILENO);
            close(fd_table.front().input_fd);

        }
        // 
        if(fd_table.front().error_fd !=STDERR_FILENO){
            dup2(fd_table.front().output_fd, STDOUT_FILENO);
            dup2(fd_table.front().error_fd, STDERR_FILENO);
            close(fd_table.front().output_fd);
            close(fd_table.front().error_fd);
        }

        if(fd_table.front().output_fd !=STDOUT_FILENO){
            dup2(fd_table.front().output_fd, STDOUT_FILENO);
            close(fd_table.front().output_fd);

        }

        
        for(int i=0;i<pipe_fd1.size();i++){
            close(pipe_fd0[i]);
            close(pipe_fd1[i]);
        }
        char ** argv;
        argv = new char * [command.size()];
        for(int i=0;i<command.size();i++)
            argv[i] = new char [256];
        // command group with ">"
        // ex: ls > test.txt
        auto it = std::find(command.begin(), command.end(), ">");
        if(it!=command.end()){
            auto idx = std::distance(command.begin(), it);

            for(int i=0;i<idx;i++){
                argv[i] = (char *) command[i].c_str();
            }
            argv[idx]=NULL;
            std::string f1(command[idx+1].c_str());
            close(1);
            int file_fd = open(f1.c_str(),  O_WRONLY | O_CREAT | O_TRUNC, 0666);
            dup2(file_fd, STDOUT_FILENO);
            execvp(argv[0], argv);
            std::string failure_message = command[0] ;
            char  temp[failure_message.size()+1];
            strcpy(temp, failure_message.c_str());
            fprintf(stderr, "Unknown command: [%s].\n", temp);
            exit(-1);
        }
        // command group without ">"
        // ex: cat test.txt
        int idx;
        for(idx=0;idx<command.size();idx++){
            argv[idx] = (char *)command[idx].c_str();
        }
        argv[idx] = NULL;
        execvp(argv[0], argv);


        // Unknown command case
        // case 1: ctt | ls
        // case 2: ctt ! ls

        std::string failure_message = command[0] ;
        char  temp[failure_message.size()+1];
        strcpy(temp, failure_message.c_str());
        fprintf(stderr, "Unknown command: [%s].\n", temp);
        exit(-1);
    }
    return pid;

}



