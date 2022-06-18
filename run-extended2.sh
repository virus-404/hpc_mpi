#!/bin/bash

## Specifies the interpreting shell for the job.
#$ -S /bin/bash

## Specifies that all environment variables active within the qsub utility be exported to the context of the job.
##$ -V  

## Execute the job from the current working directory.
#$ -cwd

## Parallel programming environment (mpich) to instantiate and number of computing slots.
#$ -pe mpich-smp 8

## The  name  of  the  job.
#$ -N AGB22_h

## Send an email at the start and the end of the job.
#$ -m ea

## The email to send the queue manager notifications. 
#$ -M agb22@alumnes.udl.cat

## The folders to save the standard and error outputs.
#$ -o $HOME/hybrid/output
#$ -e $HOME/hybrid/output

MPICH_MACHINES=$TMPDIR/mpich_machines
cat $PE_HOSTFILE | awk '{print $1":"$2}' > $MPICH_MACHINES


## In this line you have to write the command that will execute your application.
mpiexec -f $MPICH_MACHINES -n $NSLOTS ./gameoflife
rm -rf $MPICH_MACHINES

