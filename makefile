all: 
	mpicc -o gameoflife gameoflife.c ;
	mpiexec -n 4 ./gameoflife;
3: 
	mpicc -o gameoflife gameoflife.c ;
	mpiexec -n 3 ./gameoflife;
example: 
	mpicc -o struct struct.c ;
	mpiexec -n 2 ./struct;
compile: 
	mpicc -o gameoflife gameoflife.c ;
clean: 
	rm gameoflife; 
execute: 
	mpiexec -n 4 ./gameoflife;