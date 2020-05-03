BOOST = /usr/lib/x86_64-linux-gnu/libboost_system.a /usr/lib/x86_64-linux-gnu/libboost_iostreams.a /usr/lib/x86_64-linux-gnu/libboost_filesystem.a
FLAGS = -pthread
CC = g++
MCC = mpic++

cpp:
	$(CC) mr-pr-cpp.cpp $(BOOST) $(FLAGS) -o cpp_exec

mpi:
	$(MCC) mr-pr-mpi.cpp -o mpi_exec

mpi_base:
	