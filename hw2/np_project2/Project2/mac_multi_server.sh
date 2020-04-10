#! /bin/bash

cp /bin/ls /bin/cat ./bin/
g++ ./commands/removetag.cpp -o ./bin/removetag
g++ ./commands/removetag0.cpp -o ./bin/removetag0
g++ ./commands/noop.cpp -o ./bin/noop
g++ ./commands/number.cpp -o ./bin/number


make NP_multi
#g++ -std=c++11 user.cpp multi_shell.cpp multi_server.cpp -o NP_multi_server

