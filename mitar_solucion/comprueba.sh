#!/bin/bash

if ! test -x ./mytar; then
	echo "Error: mitar no está en el directorio actual"
	exit 1
fi

if test -e tmp; then
	if test -d tmp; then
		rm -rf tmp
	else
		exit 1
	fi
fi

mkdir tmp || exit 1
cd tmp
echo 'Hola mundo!' > fich1.txt
head -n 10 /etc/passwd > fich2.txt
head -c 1024 /dev/urandom > fich3.dat
if ! ../mytar -cf fichtar.mtar fich1.txt fich2.txt fich3.dat; then
	echo "Error ejecutando mitar"
	cd ..
	exit 1
fi

mkdir out
cp fichtar.mtar out
cd out
../../mytar -xf fichtar.mtar
if ! test -e fich1.txt || ! diff fich1.txt ../fich1.txt; then 
	echo "Error en extracción de fich1.txt"
	cd ../..
	exit 1
fi
if ! test -e fich2.txt || ! diff fich2.txt ../fich2.txt; then 
	echo "Error en extracción de fich2.txt"
	cd ../..
	exit 1
fi
if ! test -e fich3.dat || ! diff fich3.dat ../fich3.dat; then 
	echo "Error en extracción de fich3.dat"
	cd ../..
	exit 1
fi

cd ../..
echo "Correcto"
exit 0
