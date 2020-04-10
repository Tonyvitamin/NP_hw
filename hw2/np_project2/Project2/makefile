all:NP_simple NP_single NP_multi

NP_simple:simple_server.cpp simple_shell.cpp
	g++ -std=c++11 simple_server.cpp simple_shell.cpp -o np_simple

NP_single:single_server.cpp single_shell.cpp
	g++ -std=c++11 single_server.cpp single_shell.cpp -o np_single_proc

NP_multi:user.cpp multi_server.cpp multi_shell.cpp
	g++ -std=c++11 user.cpp multi_server.cpp multi_shell.cpp -o np_multi_proc

clean:
	rm -f np_simple, np_single_proc, np_multi_proc
