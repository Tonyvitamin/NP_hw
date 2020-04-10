#include "single_shell.h"


std::vector< std::deque<fd_info> > shell::fd_table(32, std::deque<fd_info>(1500, fd_info()));

std::deque<int>shell::pipe_fd0;
std::deque<int>shell::pipe_fd1;
client shell::users[32];

// all elements initialized as STDIN_FILENO
int shell::user_pipe_in[31][31] = { {STDIN_FILENO} };
int shell::user_pipe_out[31][31]= { {STDIN_FILENO} };

environ_set shell::env_table[31];

// loop set env every time
void shell::user_env(int sockfd){
    clearenv();
    int current_user = find_user_byfd(sockfd);
    for(int i=0;i<env_table[current_user].env.size(); i++){
        setenv(env_table[current_user].env[i].c_str(), env_table[current_user].value[i].c_str(), 1);
    }
    return;
}
// initialize all users information
void shell::init_users(){
    for(int i=1;i<31;i++){
        client user1;
        user1.id = -1;
        user1.fd = -1;
        user1.port = -1;
        bzero(user1.ip, sizeof(user1.ip));
        user1.nickname = "(no name)";
        users[i] = user1;
        env_table[i].env.push_back(std::string("PATH"));
        env_table[i].value.push_back(std::string("bin:."));
    }
    return;
}

void shell::rwg_loop(int port_n){

    init_users();

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
    int port = port_n;


    struct sockaddr_in server_info;
    bzero(&server_info, sizeof(server_info));
    server_info.sin_family = PF_INET;
    server_info.sin_addr.s_addr = INADDR_ANY;
    server_info.sin_port = htons(port);

    bind(sockfd, (struct sockaddr *)&server_info, sizeof(server_info));
    listen(sockfd, 128);

    fd_set rfds, mas;
    FD_ZERO(&rfds);
    FD_ZERO(&mas);
    FD_SET(0, &mas);
    FD_SET(sockfd, &mas);
    char buff[1024];
    int maxfd = sockfd;
    while(1){
        int flag;
        rfds = mas;
        while(select(maxfd+1, &rfds, NULL, NULL, NULL)<0){
            std::cerr<<errno<<std::endl;
        }
        for(int i=0;i<=maxfd;i++){

            if(FD_ISSET(i, &rfds)){
                bzero(buff, sizeof(buff));
                if(i==0){
                    fgets(buff, 1024, stdin);
                    if(strncmp(buff, "exit", 4)==0){
                        FD_CLR(0, &rfds);
                        exit(0);
                    }
                }
                // connection 
                // an user enters this rwg server
                else if(i==sockfd) {
                    struct sockaddr_in client_info;
                    int clientfd;
                    socklen_t len = sizeof(client_info);
                    std::cout<<i<<std::endl;
                    if((clientfd = accept(sockfd, (struct sockaddr * )&client_info, &len ))<0){
                        std::cerr<<"Fail to connect"<<std::endl;
                        exit(0);
                    }
                    std::cout<<clientfd<<std::endl;
                    
                    client user1;
                    user1.fd = clientfd;
                    strcpy(user1.ip, inet_ntoa(client_info.sin_addr));
                    user1.port = ntohs(client_info.sin_port);
                    int pos =-1;
                    for(int j=1;j<31;j++){
                        if(users[j].id == -1){
                            pos = j;
                            break; 
                        }
                    }
                    user1.id = pos;

                    std::cout<<pos<<std::endl;

                    users[pos] = user1;
                    maxfd = (clientfd > maxfd)?clientfd:maxfd;
                    std::string welcome = "****************************************\n** Welcome to the information server. **\n****************************************\n";
                    std::string come_in = "*** User '"+ user1.nickname + "' entered from "+ user1.ip + ":"+ std::to_string(user1.port)+ ". ***\n";

                    std::cout<<std::flush;
                    std::cout<<pos<<" "<<come_in<<std::endl<<std::flush;
                    send(clientfd, welcome.c_str(), welcome.length(),0);
                    broadcast(come_in);
                    send(clientfd,"% ", 2, 0);
                    FD_SET(clientfd, &mas);
                }
                // other socket action
                // an user typed the command
                else if(i>2&&((flag = find_user_byfd(i))!=-1)){
                    std::string temp;
                    int pos ;
                    int saved_stdin = dup(0);
                    int saved_stdout = dup(1);
                    int saved_stderr = dup(2);

                    int current_user = flag;
                    std::cout<<"user "<<users[current_user].id<<"typing command\n";
                    dup2(i, STDIN_FILENO);
                    dup2(i, STDOUT_FILENO);
                    dup2(i, STDERR_FILENO);
                    int check = NPloop(i);
                    if(check){
                        FD_CLR(i, &mas);
                    }
                    // restore std input, output, error
                    dup2(saved_stdin, 0);
                    dup2(saved_stdout, 1);
                    dup2(saved_stderr, 2);
                    close(saved_stdin);
                    close(saved_stdout);
                    close(saved_stderr);
                    if(check)
                        std::cout<<i<<" log out "<<std::endl;

                }

            }
        }
    }
    close(sockfd);
    return;
}

void shell::NPtell(int sockfd, int target_id, std::string msg){
    int idx = find_user_byfd(sockfd);
    fd_table[idx].pop_front();
    for(int i=1;i<31;i++){
        if(users[i].id == target_id){
            int me = find_user_byfd(sockfd);
            msg = "*** " + users[me].nickname +" told you ***: " + msg + "\n" ;
            send(users[i].fd, msg.c_str(), msg.length(), 0);
            return;
        }
    }
    std::cerr<<"*** Error: user #" << target_id <<  " does not exist yet. ***\n";
    return;
}

void shell::NPyell(int sockfd, std::string msg){
    int idx = find_user_byfd(sockfd);
    fd_table[idx].pop_front();
    int i =find_user_byfd(sockfd);
    msg = "*** "+users[i].nickname+" yelled ***:"+msg+"\n";
    broadcast(msg);
    return;
}

void shell::NPname(int sockfd, std::string name){
    int idx = find_user_byfd(sockfd);
    fd_table[idx].pop_front();

    for(int i=1;i<31;i++){
        if(users[i].nickname == name){
            std::cerr<<"*** User '"<<name<<"' already exists. ***\n";
            return;
        }
    }
    for(int i=1;i<31;i++){
        if(users[i].fd == sockfd){
            users[i].nickname = name;
            std::string name_declaration = "*** User from "+  std::string(users[i].ip) + ":" + std::to_string(users[i].port) + " is named '" + name + "'. ***\n";
            broadcast(name_declaration);
            return;
        }
    }
    return;
}

void shell::NPwho(int sockfd){
    int idx = find_user_byfd(sockfd);
    fd_table[idx].pop_front();

    std::string who_first_line = "<ID>  <nickname>  <IP:port>       <indicate me>\n";
    send(sockfd, who_first_line.c_str(), who_first_line.length(), 0);

    for(int i=1;i<31;i++){
        std::string current_user;
        if(users[i].fd == sockfd)
            current_user = std::to_string(users[i].id) + "    " + users[i].nickname + "   " + users[i].ip + ":" + std::to_string(users[i].port) + "  <-me\n";
        else if(users[i].id==-1){
            continue;
        }
        else
            current_user = std::to_string(users[i].id) + "    " + users[i].nickname + "   " + users[i].ip + ":" + std::to_string(users[i].port) + "\n";
        
        send(sockfd, current_user.c_str(), current_user.length(), 0);
    }
    return;
}

// According socket file descriptor find the corresponding user
int shell::find_user_byfd(int sockfd){
    for(int i=1;i<31;i++){
        if(users[i].fd == sockfd){
            return i;
        }
    }
    return -1;
}
int shell::find_user_byid(int id){
    for(int i=1;i<31;i++){
        if(users[i].id == id){
            return i;
        }
    }
    return -1;
}
// Erase the user according to the socket file descriptor
void shell::erase_user(int sockfd){
    for(int i=1;i<31;i++){
        if(users[i].fd == sockfd){
            // clear user info
            users[i].id = -1;
            users[i].fd -1;
            bzero(users[i].ip, sizeof(users[i].ip));
            users[i].nickname = "(no name)";
            users[i].port = -1;

            // clear env info
            env_table[i].env.clear();
            env_table[i].value.clear();
            env_table[i].env.push_back("PATH");
            env_table[i].value.push_back("bin:.");

            // clear all pipes
            // normal pipes & user pipes
            // case 1: user pipes
            for(int x=1;x<31;x++){
                for(int y=1;y<31;y++){
                    if(x==i||y==i){
                        if(user_pipe_in[x][y]!=STDIN_FILENO){
                            auto it = std::find(pipe_fd0.begin(), pipe_fd0.end(), user_pipe_in[x][y]);
                            auto idx = std::distance(pipe_fd0.begin(), it);

                            close(pipe_fd0[idx]);
                            close(pipe_fd1[idx]);
                            pipe_fd0.erase(pipe_fd0.begin()+idx);
                            pipe_fd1.erase(pipe_fd1.begin()+idx);

                            user_pipe_in[x][y] = STDIN_FILENO;
                            user_pipe_out[x][y] = STDIN_FILENO;
                        }

                    }

                }
            }
            // case 2: normal pipes
            for(int z=0;z<fd_table[i].size();z++){
                if(fd_table[i][z].input_fd != STDIN_FILENO){
                    auto it = std::find(pipe_fd0.begin(), pipe_fd0.end(), fd_table[i][z].input_fd);
                    auto idx = std::distance(pipe_fd0.begin(), it);

                    close(pipe_fd0[idx]);
                    pipe_fd0.erase(pipe_fd0.begin()+idx);
                    fd_table[i][z].input_fd = STDIN_FILENO;
                }
                if(fd_table[i][z].output_fd != STDOUT_FILENO){
                    auto it = std::find(pipe_fd1.begin(), pipe_fd1.end(), fd_table[i][z].output_fd);
                    auto idx = std::distance(pipe_fd1.begin(), it);

                    close(pipe_fd1[idx]);
                    pipe_fd1.erase(pipe_fd1.begin()+idx);
                    fd_table[i][z].output_fd = STDOUT_FILENO;
                }
                if(fd_table[i][z].error_fd != STDERR_FILENO){
                    fd_table[i][z].error_fd = STDERR_FILENO;
                }
            }

        }
    }
    close(sockfd);
    return;
}

void shell::broadcast(std::string words){
    for(int i=1;i<31;i++){
        if(users[i].id != -1){
            send(users[i].fd, words.c_str(), words.length(), 0);
        }
    }
    return;
}


int  shell::NPloop(int sockfd){

        user_env(sockfd);
        // Encounter EOF 
        std::string line;
        if(!std::getline(std::cin, line)){
            NPexit(sockfd);
            return 1;
        }

        std::string raw(line);
        std::istringstream separate(raw);
        std::string precision;
        separate>>precision;
        if(raw.size() > 15000){
            fprintf(stderr, "Unable to input too words many in a line\n");
        }
        /* built-in command */
        if(precision=="printenv" && raw.find("printenv")!=std::string::npos){
            std::istringstream s(raw);
            std::string path;
            s>>path;
            s>>path;
            NPprintenv(path, sockfd);

        }
        else if(precision=="setenv" &&  raw.find("setenv")!=std::string::npos){
            std::istringstream s(raw);
            std::string env, value;
            s>>env;
            s>>env;
            s>>value;
            NPsetenv(env, value, sockfd);
        }
        else if(precision=="exit" && raw.find("exit")!=std::string::npos){
            NPexit(sockfd);
            return 1;
        }
        else if(precision=="yell" && raw.find("yell")!=std::string::npos){
            std::string msg = raw.substr(4, raw.size()-4);

            NPyell(sockfd, msg);
        }
        else if(precision=="who" && raw.find("who")!=std::string::npos){
            NPwho(sockfd);
        }
        else if(precision=="name" && raw.find("name")!=std::string::npos){
            std::istringstream s(raw);
            std::string name;
            s>>name;
            s>>name;
            NPname(sockfd, name);
        }
        else if(precision=="tell" && raw.find("tell")!=std::string::npos){
            std::istringstream s(raw);
            std::string msg;
            int target_id;
            s>>msg;
            s>>target_id;
            getline(s, msg);
            NPtell(sockfd, target_id, msg);
        }
        /* exec-liked command*/
        else if(!raw.empty() && raw != "\n"&& raw!="\r"){
            parse(raw, sockfd);
        
        }
        int index = find_user_byfd(sockfd);
        while(fd_table[index].size() < PIPE_NUM_MAX){
            fd_table[index].push_back(fd_info());
        }
        //std::cout.flush();
        std::cerr.flush();
        //std::cout<<"% "<<std::flush;
        send(sockfd, "% ", 2, MSG_TRUNC);

    return 0;
}
void shell::parse(std::string input, int sockfd){

    /* strtok to parse the command arguments*/
    std::vector<std::string> cmdtowork;
    std::string pipe_command;
    char * tmpforinput;
    tmpforinput = new char [1024];
	int cmt=1;
	tmpforinput = strtok((char*)input.c_str() , " \t\n\r");
 
    pipe_command = tmpforinput;
	cmdtowork.push_back(strdup(tmpforinput));

    while((tmpforinput = strtok(NULL , " \t\n\r"))!=NULL){
		cmdtowork.push_back(strdup(tmpforinput));
        pipe_command = pipe_command + " " + strdup(tmpforinput);
		cmt++;

	}

    int command_group_num = 0; 
    // check parse result and set pipe number and redirection info
    std::vector<std::string> group;
    bool lineEndsWithPipeN = false;
    std::regex reg1("\\|\\d*");
    std::regex reg2("!\\d*");
    std::regex reg_pipe_in("<\\d+");
    std::regex reg_pipe_out(">\\d+");

    std::regex reg_user_pipe_in("<\\d+");
    std::regex reg_user_pipe_out(">\\d+");
    int current_user_idx = find_user_byfd(sockfd);
    int current_user = users[current_user_idx].id;
    std::string user_pipe_out_message;
    std::string user_pipe_in_message;
    bool w_user, r_user;
    w_user = false;
    r_user = false;
    for(int i = 0 ;i<cmt;i++){

        std::string tmp(cmdtowork[i]);
        //case >n or <n
        if(std::regex_match(tmp, reg_user_pipe_in) || std::regex_match(tmp, reg_user_pipe_out)){
                if(tmp[0]==(char ) '>'){
                    std::string latter_userd = tmp;
                    tmp.erase(0,1);
                    std::istringstream ss(tmp);
                    int pipe_end;
                    ss>>pipe_end;
                    //std::cout<<"current user is "<<current_user<<" pipe_end is "<<pipe_end<<std::endl;
                    if(find_user_byid(pipe_end)==-1){
                        std::cout<<"*** Error: user #"<<pipe_end<<" does not exist yet. ***\n";
                        fd_table[current_user].pop_front();
                        continue;
                    }
                    else if(user_pipe_out[current_user][pipe_end] !=STDIN_FILENO){
                        std::cout<<"*** Error: the pipe #"<<current_user<<"->#"<<pipe_end<<" already exists. ***\n";
                        fd_table[current_user].pop_front();
                        continue;
                         
                    }
                    else{
                        int pipe_fd[2];
                        pipe(pipe_fd);
                        user_pipe_out[current_user][pipe_end] = pipe_fd[1];
                        user_pipe_in[current_user][pipe_end] = pipe_fd[0];
                        std::string this_command;
                        for(int i=0;i<group.size();i++){
                            this_command = this_command + group[i] + " ";
                        }
                        fd_table[current_user].front().input_fd = (fd_table[current_user].front().input_fd!=STDIN_FILENO)?fd_table[current_user].front().input_fd: STDIN_FILENO;
                        fd_table[current_user].front().output_fd = user_pipe_out[current_user][pipe_end];
                        //fd_table[current_user].front().error_fd =  STDERR_FILENO;
                        //std::cout<<"current user is "<<current_user<<" "<<fd_table[current_user].front().input_fd<<" "<<fd_table[current_user].front().output_fd<<" "<<fd_table[current_user].front().error_fd<<std::endl;

                        pipe_fd0.push_back(pipe_fd[0]);
                        pipe_fd1.push_back(pipe_fd[1]);

                        this_command = this_command + latter_userd;
                        //user_pipe_command[current_user][pipe_end] = this_command;
                        user_pipe_out_message = "*** "+ users[current_user_idx].nickname +" (#" + std::to_string(current_user) +") just piped '"+ pipe_command + "' to " +users[find_user_byid(pipe_end)].nickname +" (#" + std::to_string(pipe_end) +") ***\n";
                        //broadcast(user_pipe_out_message);
                        w_user = true;
                    }


                }
                else {
                    std::string latter_userd = tmp;
                    tmp.erase(0,1);
                    std::istringstream ss(tmp);
                    int pipe_end;
                    ss>>pipe_end;
                    if(find_user_byid(pipe_end)==-1){
                        std::cout<<"*** Error: user #"<<pipe_end<<" does not exist yet. ***\n";
                        fd_table[current_user].pop_front();
                        continue;
                    }
                    else if(user_pipe_out[pipe_end][current_user] ==STDIN_FILENO){
                        std::cout<<"*** Error: the pipe #"<< pipe_end <<"->#"<< current_user <<" does not exist yet. ***\n";
                        fd_table[current_user].pop_front();
                        continue;
                         
                    }
                    else{
                        std::string this_command;
                        for(int i=0;i<group.size();i++){
                            this_command = this_command + group[i] + " ";
                        }
                        fd_table[current_user].front().input_fd = user_pipe_in[pipe_end][current_user];
                        user_pipe_in[pipe_end][current_user] = 0;
                        user_pipe_out[pipe_end][current_user] = 0;
                        this_command = this_command + latter_userd;
                        //user_pipe_command[current_user][pipe_end] = this_command;

                        user_pipe_in_message = "*** "+ users[current_user_idx].nickname +" (#" + std::to_string(current_user) +") just received from " + users[find_user_byid(pipe_end)].nickname +" (#" + std::to_string(pipe_end) +") by '" + pipe_command + "'  ***\n";
                        //    broadcast(user_pipe_in_message);
                        //user_pipe_command[pipe_end][current_user] = "";
                        r_user = true;
                    }
                }
                if(i==cmt-1){
                    if(r_user){
                        broadcast(user_pipe_in_message);
                        r_user = false;
                    }
                    if(w_user){
                    //    w_user = false;
                        broadcast(user_pipe_out_message);
                    }
                    pid_t pid = fork_process(group, sockfd);
                    if(fd_table[current_user].front().input_fd != STDIN_FILENO){
                        auto it = std::find(pipe_fd0.begin(), pipe_fd0.end(), fd_table[current_user].front().input_fd);
                        auto idx = std::distance(pipe_fd0.begin(), it);
                        //std::cout<<"command group "<<command_group_num <<" close pipe"<<pipe_fd0[idx]<<" "<<pipe_fd1[idx]<<std::endl;

                        close(pipe_fd0[idx]);
                        close(pipe_fd1[idx]);
                        pipe_fd0.erase(pipe_fd0.begin()+idx);
                        pipe_fd1.erase(pipe_fd1.begin()+idx);
                    }
		    if(w_user==false){
                    	int status;
                    	waitpid(pid, &status, 0);
                    }
		    else {
			w_user = false;
		    }
		    fd_table[current_user].pop_front();
                    fd_table[current_user].push_back(fd_info());
                }
        }

        // Case |n or !n 
        else if(std::regex_match(tmp, reg1) || std::regex_match(tmp, reg2) ){
            command_group_num++;
            
            
            // Case |n , !n, 
            if(tmp.size()>1){
                bool pipe_error = (tmp[0] == '!') ? true : false;
                
                tmp.erase(0,1);
                std::istringstream ss(tmp);
                int pipe_end;
                ss>>pipe_end;
                //std::cout<<"pipe end is "<<pipe_end<<std::endl;

                // If we need to create a pipe
                if(fd_table[current_user][pipe_end].input_fd==STDIN_FILENO){
                    int pipe_fd[2];
                    while(pipe(pipe_fd)<0){
                        usleep(1000);
                    }                 
                    //std::cout<<"command group "<<command_group_num<<" |"<<pipe_end<<" create pipe"<<pipe_fd[0]<<" "<<pipe_fd[1]<<std::endl;
                    fd_table[current_user][pipe_end].input_fd = pipe_fd[0];
                    
                    fd_table[current_user].front().input_fd = (fd_table[current_user].front().input_fd!=STDIN_FILENO)?fd_table[current_user].front().input_fd: STDIN_FILENO;
                    fd_table[current_user].front().output_fd = pipe_fd[1];
                    fd_table[current_user].front().error_fd = (pipe_error) ? pipe_fd[1] : STDERR_FILENO;

                    pipe_fd0.push_back(pipe_fd[0]);
                    pipe_fd1.push_back(pipe_fd[1]);
                }

                // using existing pipe    
                else {
                    int write_end_fd = fd_table[current_user][pipe_end].input_fd;
                    auto it = std::find(pipe_fd0.begin(), pipe_fd0.end(), write_end_fd);
                    auto idx = std::distance(pipe_fd0.begin(), it);
                    
                    
                    fd_table[current_user].front().input_fd = (fd_table[current_user].front().input_fd!=STDIN_FILENO)?fd_table[current_user].front().input_fd: STDIN_FILENO;
                    fd_table[current_user].front().output_fd = pipe_fd1[idx];
                    fd_table[current_user].front().error_fd = (pipe_error) ? pipe_fd1[idx] : STDERR_FILENO;
                }


                lineEndsWithPipeN = (i==cmt-1) ? true : false;
            }
            // Case | or !
            else{
                bool pipe_error = (tmp[0] == '!') ? true : false;

                // If we need to create a pipe
                if(fd_table[current_user][1].input_fd==STDIN_FILENO){
                    int pipe_fd[2];
                    while(pipe(pipe_fd)<0){
                        usleep(1000);
                    }
                    fd_table[current_user][1].input_fd= pipe_fd[0];
                    //std::cout<<"command group "<<command_group_num<<" | create pipe"<<pipe_fd[0]<<" "<<pipe_fd[1]<<std::endl;

                    fd_table[current_user].front().input_fd = (fd_table[current_user].front().input_fd!=STDIN_FILENO)?fd_table[current_user].front().input_fd: STDIN_FILENO;
                    fd_table[current_user].front().output_fd = pipe_fd[1];
                    fd_table[current_user].front().error_fd = (pipe_error) ? pipe_fd[1] : STDERR_FILENO;

                    pipe_fd0.push_back(pipe_fd[0]);
                    pipe_fd1.push_back(pipe_fd[1]);
                }

                // using existing pipe
                else {
                    int write_end_fd = fd_table[current_user][1].input_fd;
                    auto it = std::find(pipe_fd0.begin(), pipe_fd0.end(), write_end_fd);
                    auto idx = std::distance(pipe_fd0.begin(), it);
                    
                    
                    fd_table[current_user].front().input_fd = (fd_table[current_user].front().input_fd!=STDIN_FILENO)?fd_table[current_user].front().input_fd: STDIN_FILENO;
                    fd_table[current_user].front().output_fd = pipe_fd1[idx];
                    fd_table[current_user].front().error_fd = (pipe_error) ? pipe_fd1[idx] : STDERR_FILENO;                
                }
                


                lineEndsWithPipeN = (i==cmt-1) ? true : false;
            }
            if(r_user){
                broadcast(user_pipe_in_message);
                r_user = false;
            }
            pid_t pid = fork_process(group, sockfd);
            if(fd_table[current_user].front().input_fd != STDIN_FILENO){
                auto it = std::find(pipe_fd0.begin(), pipe_fd0.end(), fd_table[current_user].front().input_fd);
                auto idx = std::distance(pipe_fd0.begin(), it);
                //std::cout<<"command group "<<command_group_num<<" close pipe"<<pipe_fd0[idx]<<" "<<pipe_fd1[idx]<<std::endl;

                close(pipe_fd0[idx]);
                close(pipe_fd1[idx]);
                pipe_fd0.erase(pipe_fd0.begin()+idx);
                pipe_fd1.erase(pipe_fd1.begin()+idx);
            }

            fd_table[current_user].pop_front();
            fd_table[current_user].push_back(fd_info());
            

            group.clear();
        }
        // Last command in the line(except |n, !n, |, !)
        else if(i==cmt-1){
            if(r_user){
                broadcast(user_pipe_in_message);
                r_user=false;
            }
            group.push_back(tmp);
            pid_t pid = fork_process(group, sockfd);
            if(fd_table[current_user].front().input_fd != STDIN_FILENO){
                auto it = std::find(pipe_fd0.begin(), pipe_fd0.end(), fd_table[current_user].front().input_fd);
                auto idx = std::distance(pipe_fd0.begin(), it);
                //std::cout<<"command group "<<command_group_num <<" close pipe"<<pipe_fd0[idx]<<" "<<pipe_fd1[idx]<<std::endl;

                close(pipe_fd0[idx]);
                close(pipe_fd1[idx]);
                pipe_fd0.erase(pipe_fd0.begin()+idx);
                pipe_fd1.erase(pipe_fd1.begin()+idx);
            }

            fd_table[current_user].pop_front();
            fd_table[current_user].push_back(fd_info());
            if(!lineEndsWithPipeN){
                int status;
                waitpid(pid, &status, 0);
                //std::cout<<"waitpid catch child "<< pid<<std::endl;
            }
        }
        else
            group.push_back(tmp);
    }
    cmdtowork.clear();
    delete [] tmpforinput;
    return;

}

void shell::NPprintenv(std::string input, int sockfd){
    const char * c_input = input.c_str();
    char * path = getenv(c_input);
    if(path != NULL){
        fprintf(stdout,"%s\n", path);
    }
    int current_user_idx = find_user_byfd(sockfd);
    int current_user = users[current_user_idx].id;
    fd_table[current_user].pop_front();

    return ;
}

void shell::NPexit(int sockfd){
    int idx = find_user_byfd(sockfd);
    std::string msg =  "*** User '" + users[idx].nickname +  "' left. ***\n";
    broadcast(msg);
    //shutdown(sockfd, 2);
    erase_user(sockfd);
    return;
}

void shell::NPsetenv(std::string env, std::string path, int sockfd){
    const char * name = env.c_str();
    const char * value = path.c_str();
    int overwrite = 1;

    int check = setenv(name, value, overwrite);
    int current_user = find_user_byfd(sockfd);
    auto it = std::find(env_table[current_user].env.begin(), env_table[current_user].env.end(), name);
    if(it!=env_table[current_user].env.end()){
        auto idx = std::distance(env_table[current_user].env.begin(), it);
        env_table[current_user].value[idx] = path;
    }
    else {
        env_table[current_user].env.push_back(env);
        env_table[current_user].value.push_back(path);
    }
    fd_table[current_user].pop_front();

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

pid_t shell::fork_process(std::vector< std::string> command, int sockfd){
    signal(SIGCHLD, childHandler);

    pid_t pid;
    while((pid=fork())<0){
        usleep(1000);
    }
    //std::cout<<fd_table.size()<<" "<<pipe_fd0.size()<<" "<< pipe_fd1.size()<<std::endl;
    // Child process 
    // handle exec like command
    if(pid==0){         
        int current_user_idx = find_user_byfd(sockfd);
        int current_user = users[current_user_idx].id;
        //std::cout<<current_user<<" "<<getppid()<<" "<<getpid()<<" "<<fd_table[current_user].front().input_fd<<" "<<fd_table[current_user].front().output_fd<<" "<<fd_table[current_user].front().error_fd<<std::endl;

        // |n or !n target command
        if(fd_table[current_user].front().input_fd != STDIN_FILENO){
            dup2(fd_table[current_user].front().input_fd, STDIN_FILENO);
            close(fd_table[current_user].front().input_fd);

        }
        // 
        if(fd_table[current_user].front().error_fd !=STDERR_FILENO){
            dup2(fd_table[current_user].front().output_fd, STDOUT_FILENO);
            dup2(fd_table[current_user].front().error_fd, STDERR_FILENO);
            close(fd_table[current_user].front().output_fd);
            close(fd_table[current_user].front().error_fd);
        }

        if(fd_table[current_user].front().output_fd !=STDOUT_FILENO){
            dup2(fd_table[current_user].front().output_fd, STDOUT_FILENO);
            close(fd_table[current_user].front().output_fd);

        }

        
        for(int i=0;i<pipe_fd1.size();i++){
            close(pipe_fd0[i]);
            close(pipe_fd1[i]);
        }
        /*
        for(int i=0;i<30;i++){
            for(int j=0;j<30;j++){
                if(user_pipe_out[i][j]!=STDIN_FILENO)
                    close(user_pipe_out[i][j]);
                if(user_pipe_in[i][j]!=STDIN_FILENO)
                    close(user_pipe_in[i][j]);           
            }
        }*/
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



