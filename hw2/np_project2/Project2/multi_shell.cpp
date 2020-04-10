#include "multi_shell.h"

//int user_pipe_fd_in[31][31];
//int user_pipe_fd_out[31][31];

std::deque<fd_info> shell::fd_table(1500);
std::deque<int>shell::pipe_fd0;
std::deque<int>shell::pipe_fd1;

void User_pipeHandler(int signo){
    message * line = get_message(9487);
    std::string tmp_string(line->msg);
    std::istringstream ss(tmp_string);
    int sender, receiver;
    ss>>sender;
    ss>>receiver;
    std::string FIFO = "./user_pipe/"+std::to_string(sender) + "->"+std::to_string(receiver);
    detach_msg_mem(line);
    //std::cout<<FIFO.c_str()<<std::endl;
    user_pipe_fd_in[sender][receiver] = open(FIFO.c_str(), O_RDONLY);
    //std::cout<<"user pipe read end created "<<sender<<" "<<receiver<<" "<<user_pipe_fd_in[sender][receiver]<<std::endl;
}

void MSGHandler(int signo){
    message * line = get_message(9487);
    std::cout<<line->msg<<std::flush;
    detach_msg_mem(line);
}

int user_login(key_t client_key, key_t msg_key, int sockfd, struct sockaddr_in client_info){
    client * users = get_client_list(client_key);
    int idn,  portn;
    pid_t pidn;
    std::string name = "(no name)";
    char ipaddr[16];
    int pos = -1;

    for(int i=0;i<30;i++){
        if(users[i].id == -1){
            pos = i;
            break;
        }
    }
    idn = pos + 1;
    pidn = getpid();
    portn = ntohs(client_info.sin_port);
    strcpy(ipaddr, inet_ntoa(client_info.sin_addr));
    client new_user(idn, pidn, name, portn, ipaddr);
    users[pos] = new_user;
    detach_client_mem(users);

    std::string welcome = "****************************************\n** Welcome to the information server. **\n****************************************\n";
    std::string come_in = "*** User '"+ std::string(new_user.nickname) + "' entered from "+ new_user.ip + ":"+ std::to_string(new_user.port)+ ". ***\n";

    send(sockfd, welcome.c_str(), welcome.length(),0);
    broadcast(come_in, client_key, msg_key);
    send(sockfd, "% ", 2, 0);


    return idn;
}

void user_logout(key_t key){

    client  *data = get_client_list(8787);

    int pos;
    std::string user_name;
    int index = find_user_bypid(getpid());
    for(int i=0;i<30;i++){
        if(data[i].id != -1){
            if(data[i].pid == getpid()){
                user_name = data[i].nickname;
                pos = i;
                break;
                //client user;
                //data[i] = user;
            }

        }
    }
    // unlink 58
    for(int i =0;i<30;i++){
        if(data[i].id != -1){
            if(user_pipe_fd_in[i][data[pos].id]!=-1){
                close(user_pipe_fd_in[i][data[pos].id]);
                std::string FIFO_link_1 = "./user_pipe/" + std::to_string(data[i].id) + "->" +std::to_string(data[pos].id);
                unlink(FIFO_link_1.c_str());

            }
        }
        std::string FIFO_link_1 = "./user_pipe/" + std::to_string(i+1) + "->" +std::to_string(data[pos].id);
        unlink(FIFO_link_1.c_str());
    }
    std::string words = "*** User '" + user_name + "' left. ***\n"; 
    broadcast(words, 8787, 9487);
    client user = client();
    data[pos] = user;
    detach_client_mem(data);

    return;
}

void broadcast(std::string words, key_t client_key, key_t msg_key){
    message * line = get_message(msg_key);
    strcpy(line->msg, words.c_str());
    detach_msg_mem(line);

    client * users = get_client_list(client_key);

    for(int i=0;i<30;i++){
        if(users[i].pid != -1){
            kill(users[i].pid, SIGUSR1);
            //std::cout<<"broadcast "<<users[i].pid<<" success\n";
        }
    }

    detach_client_mem(users);

}
void shell::NPtell(std::string words, int target_id){
    fd_table.pop_front();
    client * users = get_client_list(8787);
    int index = find_user_bypid(getpid());
    std::string current_user = users[index].nickname;
    std::string word_model = "*** "  + current_user + " told you ***:" + words + "\n";
    for(int i =0;i<30;i++){
        if(users[i].id == target_id){
            message * line = get_message(9487);
            strcpy(line->msg, word_model.c_str());
            if (shmdt((void *) line) == -1) {
                perror("shmdt\n");
            }
            kill(users[i].pid, SIGUSR1);

            if (shmdt((void *) users) == -1) {
                perror("shmdt\n");
            }
            return;
        }
    }
    std::cerr<<"*** Error: user #"<<target_id<<" does not exist yet. ***\n";
    detach_client_mem(users);
    return;
}

void shell::NPwho(){
    fd_table.pop_front();
    std::string who_first_line = "<ID>  <nickname>  <IP:port>       <indicate me>\n";
    std::cout<<who_first_line;
    client * users = get_client_list(8787);
    for(int i=0;i<30;i++){
        if(users[i].id !=-1){
            std::string current_user;
            if(users[i].pid == getpid())
                current_user = std::to_string(users[i].id) + "    " + std::string(users[i].nickname) + "   " + std::string(users[i].ip) + ":" + std::to_string(users[i].port) + "  <-me\n";
            else
                current_user = std::to_string(users[i].id) + "    " + std::string(users[i].nickname) + "   " + std::string(users[i].ip) + ":" + std::to_string(users[i].port) + "\n";
            std::cout<<current_user;
        }
    }
    if (shmdt((void *) users) == -1) {
        perror("shmdt\n");
    }
    return;
}

void shell::NPname(std::string words, key_t key){
    fd_table.pop_front();
    client * users = get_client_list(key);
    for(int i=0;i<30;i++){
        if(users[i].id == -1){
            continue;
        }
        else{
            if((words.compare(users[i].nickname))==0){
                std::cerr<<"*** User \'"<<words<<"\' already exists. ***\n";
                if (shmdt((void *) users) == -1) {
                    perror("shmdt\n");
                }
                return;
            }
        }
    }
    int pos = find_user_bypid(getpid());
    std::string announcement = "*** User from "+ std::string(users[pos].ip) + ":" + std::to_string(users[pos].port) + " is named \'" + words.c_str() + "\'. ***\n";
    strcpy(users[pos].nickname, words.c_str());
    if (shmdt((void *) users) == -1) {
        perror("shmdt\n");
    }
    broadcast(announcement, 8787, 9487);
    return; 
}

void shell::NPyell(std::string msg, key_t key){
    fd_table.pop_front();
    client * users = get_client_list(key);
    int i = find_user_bypid(getpid());
    msg = "*** "+std::string(users[i].nickname)+" yelled ***:"+msg+"\n";
    broadcast(msg, 8787, 9487);
    if (shmdt((void *) users) == -1) {
        perror("shmdt\n");
    }
    return;
}

void shell::NPloop(key_t key){
    for(int i=1;i<31;i++){
        for(int j=1;j<31;j++){
            user_pipe_fd_in[i][j] = -1;
            user_pipe_fd_out[i][j] = -1;
        }
    }
    for(;;){
        //char  line[LINE_len_MAX_BUFF];
        // Encounter EOF 
        std::string line;
        if(!std::getline(std::cin, line)){
            //std::cout<<"there is no input\n";
		    return;
        }

        //std::cout<<"get commands "<<line<<std::endl;
        std::string raw(line);
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
            NPexit();
            return;
        }
        else if(precision=="yell" && raw.find("yell")!=std::string::npos){
            std::string msg = raw.substr(4, raw.size()-4);

            NPyell(msg, key);
        }
        else if(precision=="tell" && raw.find("tell")!=std::string::npos){
            std::istringstream s(raw);
            std::string msg;
            int target_id;
            s>>msg;
            s>>target_id;
            getline(s, msg);
            NPtell(msg, target_id);
        }
        else if(precision=="name" && raw.find("name")!=std::string::npos){
           std::istringstream s(raw);
            std::string name;
            s>>name;
            s>>name;
            NPname(name, key);
        }
        else if(precision=="who" && raw.find("who")!=std::string::npos){
            NPwho();
        }
        /* exec-liked command*/
        else if(!raw.empty() && raw != "\n" && raw != "\r"){
            //std::cout<<"enter exec-liked\n";
            parse(line);
        
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
    std::string pipe_command = strdup(tmpforinput);
	cmdtowork.push_back(strdup(tmpforinput));
    while((tmpforinput = strtok(NULL , " \t\n\r"))!=NULL){
		cmdtowork.push_back(strdup(tmpforinput));
        pipe_command = pipe_command + " " + strdup(tmpforinput);
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
    std::regex reg_user_pipe_in("<\\d+");
    std::regex reg_user_pipe_out(">\\d+");
    client * us = get_client_list(8787);
    int current_user_id = us[find_user_bypid(getpid())].id;
    std::string current_user_name = us[find_user_bypid(getpid())].nickname;
    detach_client_mem(us);
    int prev_read_fd = -1;
    int prev_read_id = -1;
    int prev_write_fd = -1;
    int prev_write_id = -1;
    std::string pipe_out_message;
    std::string pipe_in_message;

    bool w_user, r_user;
    w_user = false;
    r_user = false;

    for(int i = 0 ;i<cmt;i++){

        std::string tmp(cmdtowork[i]);


        //case: <n or >n
        if(std::regex_match(tmp, reg_user_pipe_out) || std::regex_match(tmp, reg_user_pipe_in)){
            if(std::regex_match(tmp, reg_user_pipe_in)){
                std::string copy_one(tmp);
                copy_one.erase(0,1);
                std::istringstream ss(copy_one);
                int user_pipe_end;
                ss>>user_pipe_end;
                client * users = get_client_list(8787);
                int user_index=-1;
                for(int j = 0;j<30;j++){
                    // user not found
                    if(users[j].id==-1){
                        std::cerr<<"*** Error: user #"<<user_pipe_end<<" does not exist yet. ***\n";
                        break;
                    }
                    // user found
                    else if(users[j].id == user_pipe_end){
                        user_index = j;
                        current_user_id = users[find_user_bypid(getpid())].id;
                        current_user_name = users[find_user_bypid(getpid())].nickname;
                        // pipe not found
                        if(user_pipe_fd_in[user_pipe_end][current_user_id] == -1){
                            std::cerr<<"*** Error: the pipe #"<<std::to_string(user_pipe_end)<<"->#"<<std::to_string(current_user_id)<<" does not exist yet. ***\n";
                            break;
                        }
                        // pipe found
                        else {
                            fd_table.front().input_fd = user_pipe_fd_in[user_pipe_end][current_user_id];
                            prev_read_fd = fd_table.front().input_fd ;
                            prev_read_id = user_pipe_end;

                            pipe_in_message= "*** "+ current_user_name + " (#" + std::to_string(current_user_id)+ ") just received from " + users[user_index].nickname + " (#" + std::to_string(user_pipe_end)+") by '"+pipe_command+"' ***\n";
                            r_user = true;
                            // case 1
                            // xxx <number
                            // consideration:
                            // xxx >number <number or xxx <number
                            if(i==cmt-1){
                                //std::cout<<"sender is "<<user_pipe_end<<" receiver is "<<current_user_id<<" here\n";
                                std::string FIFO = "./user_pipe/"+std::to_string(user_pipe_end) + "->"+std::to_string(current_user_id);
                                broadcast(pipe_in_message, 8787, 9487);
				pid_t pid = fork_process(group);

                                // close read user pipe
                                close(user_pipe_fd_in[user_pipe_end][current_user_id]);
                                user_pipe_fd_in[user_pipe_end][current_user_id] = -1;

                                bool write_check = false;
                                if(prev_write_fd !=-1){
                                    write_check = true;
                                    close(prev_write_fd);
                                    user_pipe_fd_out[prev_write_id][current_user_id] = -1;
                                    prev_write_fd = -1;
                                }

                                if(unlink(FIFO.c_str())==0){
                                    //std::cout<<"delete user pipe "<<FIFO<<" success\n";
                                }


                                //broadcast(pipe_in_message, 8787, 9487);
                                if(write_check){
                                    usleep(2000);
                                    broadcast(pipe_out_message, 8787, 9487);
				}
                                else{
				    int status;
				    waitpid(pid, &status, 0);
				}
                                //int status;
                                //waitpid(pid, &status, 0);
                                //std::cout<<"waitpid catch child "<<pid<<std::endl;


                                fd_table.pop_front();
                                fd_table.push_back(fd_info());

                            }

                        }
                        break;
                    }
                }
                detach_client_mem(users);


            }
            else if(std::regex_match(tmp, reg_user_pipe_out)){
                std::string copy_one(tmp);
                copy_one.erase(0,1);
                std::istringstream ss(copy_one);
                int user_pipe_end;
                ss>>user_pipe_end;
                client * users = get_client_list(8787);
                int user_index=-1;
                for(int j = 0;j<30;j++){
                    // user not found
                    if(users[j].id==-1){
                        std::cerr<<"*** Error: user #"<<user_pipe_end<<" does not exist yet. ***\n";
                        break;
                    }
                    // user found
                    else if(users[j].id == user_pipe_end){
                        user_index = j;
                        current_user_id = users[find_user_bypid(getpid())].id;
                        current_user_name = users[find_user_bypid(getpid())].nickname;
                        std::string FIFO = "./user_pipe/"+std::to_string(current_user_id) + "->"+std::to_string(user_pipe_end);
                        int check;
                        // pipe already exist
                        if ( ((check = mknod(FIFO.c_str(), S_IFIFO | 0666, 0)) < 0) && (errno == EEXIST)){
                            std::cerr<<"*** Error: the pipe #"<<std::to_string(current_user_id) << "->#"<<std::to_string(user_pipe_end)<<" already exists. ***\n";
                            break;
                        }
                        // pipe created 
                        else
                        { 
                            // write read end pipe and write end pipe in memory
                            message * data = get_message(9487);
                            std::string out_in = std::to_string(current_user_id) + " " + std::to_string(user_pipe_end);
                            strcpy(data->msg, out_in.c_str());
                            detach_msg_mem(data);

                            // open read end's fifo
                            std::cout<<std::flush;
                            //std::cout<<find_pid_byid(user_pipe_end)<<std::endl;
                            //std::cout<<find_user_bypid(find_pid_byid(user_pipe_end))<<std::endl;
                            kill(find_pid_byid(user_pipe_end), SIGUSR2);
                            user_pipe_fd_out[current_user_id][user_pipe_end] = open(FIFO.c_str(), O_WRONLY);
                            prev_write_fd = user_pipe_fd_out[current_user_id][user_pipe_end];
                            prev_write_id = user_pipe_end;
                            //std::cout<<"write end created "<<user_pipe_fd_out[current_user_id][user_pipe_end]<<std::endl;

                            fd_table.front().output_fd = user_pipe_fd_out[current_user_id][user_pipe_end] ;    
                            //std::cout<<"pipe created\n";




                            pipe_out_message = "*** "+ current_user_name + " (#" + std::to_string(current_user_id)+ ") just piped '"+pipe_command+"' to " + users[user_index].nickname + " (#" + std::to_string(user_pipe_end)+") ***\n";

                            // case 2
                            // xxx >number
                            // consideration:
                            // xxx | xxx >number or xxx <number >number
                            if(i==cmt-1){
                                pid_t pid = fork_process(group);
                                if(prev_read_fd!=-1){
                                    close(prev_read_fd);
                                    user_pipe_fd_in[prev_read_id][current_user_id] = -1;
                                    std::string read_end_FIFO = "./user_pipe/"+std::to_string(prev_read_id)+"->"+std::to_string(current_user_id);
                                    unlink(read_end_FIFO.c_str());
                                    broadcast(pipe_in_message, 8787, 9487);
                                    //prev_read_fd = -1;
                                }
                                else if(fd_table.front().input_fd != STDIN_FILENO){
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
                                usleep(2000);
                                broadcast(pipe_out_message, 8787, 9487);  

                                int status;
                                //waitpid(pid, &status, 0);
                                //std::cout<<"waitpid catch child "<< pid<<std::endl;
                                close(user_pipe_fd_out[current_user_id][user_pipe_end]);
                                user_pipe_fd_out[current_user_id][user_pipe_end]=-1;

                            }      
                        }
                        break;
                    }
                }
                detach_client_mem(users);
                //if (shmdt((void *) users) == -1) {
                //    perror("shmdt\n");
                //}
            }

        }
        
        // Case |n, !n, |, ! 
        // case 3
        // consideration:
        // xxx <number |  or xxx | or xxx <number ! or xxx ! 
        else if(std::regex_match(tmp, reg1) || std::regex_match(tmp, reg2)){

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
            if(prev_read_fd !=-1){
                close(prev_read_fd);
                user_pipe_fd_in[prev_read_id][current_user_id] = -1;
                std::string FIFO = "./user_pipe/"+std::to_string(prev_read_id) + "->"+std::to_string(current_user_id);
                broadcast(pipe_in_message, 8787, 9487);
                unlink(FIFO.c_str());
                prev_read_fd = -1;
            }
            else if(fd_table.front().input_fd != STDIN_FILENO){
                auto it = std::find(pipe_fd0.begin(), pipe_fd0.end(), fd_table.front().input_fd);
                auto idx = std::distance(pipe_fd0.begin(), it);

                close(pipe_fd0[idx]);
                close(pipe_fd1[idx]);
                pipe_fd0.erase(pipe_fd0.begin()+idx);
                pipe_fd1.erase(pipe_fd1.begin()+idx);
            }

            fd_table.pop_front();
            fd_table.push_back(fd_info());
            

            group.clear();
        }

        // case 4
        // consideration:
        // xxx | xxx or xxx(always wait for child process)
        else if(i==cmt-1){
            group.push_back(tmp);
            pid_t pid = fork_process(group);
            if(fd_table.front().input_fd != STDIN_FILENO ){
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
    //
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

void shell::NPexit(){
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
            //std::string things;
            //while(std::cin>>things){
            //    std::cout<<things<<std::endl;
            //}

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



