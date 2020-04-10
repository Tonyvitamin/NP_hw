#! /bin/bash

cp /bin/ls /bin/cat ./bin/
cp ./figlet ./bin/
g++ ./commands/removetag.cpp -o ./bin/removetag
g++ ./commands/removetag0.cpp -o ./bin/removetag0
g++ ./commands/noop.cpp -o ./bin/noop
g++ ./commands/number.cpp -o ./bin/number

make
