#include "user.h"

void detach_client_mem(client * users){
    if (shmdt((void *) users) == -1) {
        perror("shmdt\n");
        exit(1);
    }
    return;
}

void detach_msg_mem(message * data){
    if (shmdt((void *) data) == -1) {
        perror("shmdt\n");
        exit(1);
    }
    return;
}

int find_pid_byid(int id){
    client * users = get_client_list(8787);
    for(int i=0;i<30;i++){
        if(users[i].id == id){
            pid_t pid= users[i].pid;
            detach_client_mem(users);

            return pid;
        }
    }
    return -1;
}

void create_msgMem(key_t key){
    int shmid;
    if ((shmid = shmget(key, sizeof(message), 0644|IPC_CREAT)) == -1) {
        perror("shmget");
        exit(1);
    }
    message  *data;
    data = (message *) shmat(shmid, NULL, 0);
        
    if (data == (message * )(-1)) {
        perror("shmat\n");
        exit(1);
    }
    message msg;
    strcpy(data->msg, "");

    detach_msg_mem(data);
    return;
}

struct message * get_message(key_t key){
    int shmid;
    if ((shmid = shmget(key, sizeof(message), 0644|IPC_CREAT)) == -1) {
        perror("shmget");
        exit(1);
    }
    message  *data;
    data = (message *) shmat(shmid, NULL, 0);
        
    if (data == (message * )(-1)) {
        perror("shmat\n");
        exit(1);
    }
    return data;
    /*
    if (shmdt((void *) data) == -1) {
        perror("shmdt\n");
        exit(1);
    }
    */
}

struct client * get_client_list(key_t key){
    int shmid;
    if ((shmid = shmget(key, 30*sizeof(client), 0644|IPC_CREAT)) == -1) {
        perror("shmget");
        exit(1);
    }
    client  *data;
    data = (client *) shmat(shmid, NULL, 0);
        
    if (data == (client * )(-1)) {
        perror("shmat\n");
        exit(1);
    }
    return data;
    /*
    if (shmdt((void *) data) == -1) {
        perror("shmdt\n");
        exit(1);
    }
    */
}


// create a memory space all users(30)
void create_clientMem(key_t key){
    int shmid;
    if ((shmid = shmget(key, 30*sizeof(client), 0644|IPC_CREAT)) == -1) {
        perror("shmget");
        exit(1);
    }
    client  *data;
    data = (client *) shmat(shmid, NULL, 0);
        
    if (data == (client * )(-1)) {
        perror("shmat\n");
        exit(1);
    }
    for (int i=0;i<30;i++){
        client user;
        data[i] = user;
    }
    detach_client_mem(data);
    return;
}

int find_user_bypid(pid_t pid){
    client * users = get_client_list(8787);
    for(int i =0;i<30;i++){
        if(users[i].pid == pid){
            detach_client_mem(users);
            return i;
        }
    }
    return -1;
}


// Triggered by interrupt, we should clear the shared memory
void INTHandler(int signo){
    int shmid;
    if ((shmid = shmget(8787, 30*sizeof(client), 0644|IPC_CREAT)) == -1) {
        perror("shmget");
        exit(1);
    }
    int res=shmctl(shmid,IPC_RMID,NULL); 
    if(-1==res) 
        perror("shmctl"),exit(-1); 

    if ((shmid = shmget(9487, sizeof(message), 0644|IPC_CREAT)) == -1) {
        perror("shmget");
        exit(1);
    }
    res=shmctl(shmid,IPC_RMID,NULL); 
    if(-1==res) 
        perror("shmctl"),exit(-1); 
    printf("delete success\n");
    for(int i=1;i<31;i++){
        for(int j=1;j<31;j++){
            std::string FIFO_link_1 = "./user_pipe/" + std::to_string(i) + "->" +std::to_string(j);
            unlink(FIFO_link_1.c_str());

        }
    } 

    exit(0);
}






