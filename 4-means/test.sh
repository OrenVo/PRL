#!/bin/bash

#pocet cisel bud zadam nebo 10 :)
if [ $# -lt 1 ];then 
    numbers=4;
else
    numbers=$1;
fi;

#preklad cpp zdrojaku
mpic++ --prefix /usr/local/share/OpenMPI -g -o parkmeans parkmeans.cc


#vyrobeni souboru s random cisly
dd if=/dev/random bs=1 count=$numbers of=numbers

#spusteni
mpirun --prefix /usr/local/share/OpenMPI --oversubscribe -np $numbers parkmeans

#mpirun --prefix /usr/local/share/OpenMPI -np $numbers valgrind --suppressions=/usr/local/share/OpenMPI/share/openmpi/openmpi-valgrind.supp ./parkmeans
#mpirun --prefix /usr/local/share/OpenMPI -np $numbers valgrind --suppressions=/usr/lib64/mpi/gcc/openmpi4/share/openmpi/openmpi-valgrind.supp  ./parkmeans


#uklid
# rm -f oems numbers
