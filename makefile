cluster: 
	mpicc -o gameoflife gameoflife.c ;
	qsub run-extended2.sh; 
	qsub run-extended4.sh; 
	qsub run-extended8.sh; 
local: 
	mpicc -o gameoflife gameoflife.c ;
	mpiexec -n 4 ./gameoflife;
compile: 
	mpicc -fopenmp -o gameoflife gameoflifev3.c;
execute: 
	mpiexec -n 2 ./gameoflife;
save: 
	cat output/AGB22.o* > results
hybrid: 
	mpicc -fopenmp -o gameoflife gameoflifev3.c;
	mpiexec -n 2 ./gameoflife;
	

