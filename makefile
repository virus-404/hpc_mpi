all: 
	mpicc -o gameoflife gameoflife.c ;
	mpiexec -n 5 ./gameoflife;
example: 
	mpicc -o struct struct.c ;
	mpiexec -n 2 ./struct;
compile: 
	mpicc -o gameoflife gameoflife.c ;
clean: 
	rm gameoflife struct; 
execute: 
	mpiexec -n 4 ./gameoflife;