BOOST = /usr/lib/x86_64-linux-gnu/libboost_system.a /usr/lib/x86_64-linux-gnu/libboost_iostreams.a /usr/lib/x86_64-linux-gnu/libboost_filesystem.a
FLAGS = -pthread
MRMPI = ./mr_mpi_lib/libmrmpi_linux.a
CC = g++
MCC = mpic++

all: cpp mpi mpi_base

cpp:
	$(CC) mr-pr-cpp.cpp $(BOOST) $(FLAGS) -o cpp_exec

mpi:
	$(MCC) mr-pr-mpi.cpp -o mpi_exec

mpi_base:
	$(MCC) mr-pr-mpi-base.cpp $(MRMPI) -o mpi_base_exec