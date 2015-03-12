#!/bin/bash
#Modlist should be loaded
COUNT=0
while [ $COUNT -le 10000 ]; do
	echo "add 3" > /proc/modlist
	echo "add 2" > /proc/modlist
	echo "add 3" > /proc/modlist
	cat /proc/modlist	
	echo "remove 3" > /proc/modlist
	echo "add 3" > /proc/modlist
	echo "remove 2" > /proc/modlist
	echo "cleanup" > /proc/modlist
	echo "add 1" > /proc/modlist
	echo "add 1" > /proc/modlist
	echo "add 1" > /proc/modlist
	echo "remove 1" > /proc/modlist
	echo "add 1" > /proc/modlist
	echo "add 1" > /proc/modlist
	echo "add 2" > /proc/modlist
	echo "add 3" > /proc/modlist
	echo "add 20" > /proc/modlist
	cat /proc/modlist
	let COUNT=COUNT+1
done
