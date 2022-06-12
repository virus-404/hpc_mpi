all: 
	mpicc -o gameoflife gameoflifev2.c ;
	mpiexec -n 4 ./gameoflife;
example: 
	mpicc -o struct struct.c ;
	mpiexec -n 3 ./struct;
compile: 
	mpicc -o gameoflife gameoflifev2.c ;
clean: 
	rm gameoflife; 
execute: 
	mpiexec -n 4 ./gameoflife;