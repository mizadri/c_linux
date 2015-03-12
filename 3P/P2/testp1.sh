#!/bin/bash
#Modlist should be loaded
COUNT=0
while [ $COUNT -le 10000 ]; do
	sudo insmod ModParteC/counter.ko
	sudo insmod Opcional3/modlist.ko
	sudo rmmod counter
	sudo rmmod modlist
	let COUNT=COUNT+1
done
