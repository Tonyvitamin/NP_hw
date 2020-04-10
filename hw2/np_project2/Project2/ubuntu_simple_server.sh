#! /bin/bash

cp /bin/ls /bin/cat ./bin/
g++ ./commands/removetag.cpp -o ./bin/removetag
g++ ./commands/removetag0.cpp -o ./bin/removetag0
g++ ./commands/noop.cpp -o ./bin/noop
g++ ./commands/number.cpp -o ./bin/number

make NP_simple
#g++ -std=c++11 simple_shell.cpp simple_server.cpp -o NP_simple_server

