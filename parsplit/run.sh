#!/bin/bash

#pocet cisel bud zadam nebo 10 :)
if [ $# -lt 1 ];then 
    numbers=10;
    processes=10;
else
    numbers=$1;
    processes=$2;
fi;

#preklad cpp zdrojaku
mpic++ --prefix /usr/local/share/OpenMPI -O2 -o parsplit parsplit.cpp


#vyrobeni souboru s random cisly
dd if=/dev/random bs=1 count=$numbers of=numbers

#spusteni
mpirun --prefix /usr/local/share/OpenMPI --oversubscribe -np $processes parsplit

#mpirun --prefix /usr/local/share/OpenMPI -np $processes valgrind --suppressions=/usr/local/share/OpenMPI/share/openmpi/openmpi-valgrind.supp ./parsplit

#uklid
rm -f oems numbers
