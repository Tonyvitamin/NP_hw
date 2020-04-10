#include "single_shell.h"



int main(int argc, char * argv[]){

    int port = atoi(argv[1]);
    shell npshell;
    npshell.rwg_loop(port);
    return 0;
}