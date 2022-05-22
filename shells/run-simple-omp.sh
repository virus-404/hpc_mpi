#!/bin/bash

ancho=$2
largo=$1
iteracion=$3

## Specifies the interpreting shell for the job.
#$ -S /bin/bash

## Specifies that all environment variables active within the qsub utility be exported to the context of the job.
#$ -V

## Specifies the parallel environment
#$ -pe smp 2

## Execute the job from the current working directory.
#$ -cwd 

## The  name  of  the  job.
#$ -N ompfile
## Name of resoult an error
#$ -o resul$JOB_ID.out
#$ -e error$JOB_ID.err

##send an email when the job ends
#$ -m e

##email addrees notification
#$ -M erc14@alumnes.udl.cat

##Passes an environment variable to the job
#$ -v  OMP_NUM_THREADS=2

## In this line you have to write the command that will execute your application.
./lifegame1 Examples/${ancho}x${largo}/LifeGameInit_${ancho}x${largo}_iter0.txt  ${largo} ${ancho} ${iteracion}
mv resul$JOB_ID.out resulM${largo}x${ancho}_iter${iteracion}
mv error$JOB_ID.err errorM${largo}x${ancho}_iter${iteracion}

mv resulM${largo}x${ancho}_iter${iteracion} /home/erc14/ProyectoFinal/resultados
mv errorM${largo}x${ancho}_iter${iteracion} /home/erc14/ProyectoFinal/errores




