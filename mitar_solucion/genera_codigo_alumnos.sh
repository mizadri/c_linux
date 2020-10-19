#!/bin/bash
ficherosAlumnos="mytar.c  mytar_routines.c mytar.h"
makefileAlumnos=Makefile
directorioAlumnos=${1-"../../Alumno/FicherosP1/Mytar"}
DIRCPPP=../../../common/cppp
CPPP=${DIRCPPP}/cppp

if [ ! -d "$directorioAlumnos" ]; then
	mkdir $directorioAlumnos
fi



#Build CPPP in case it is not already built
if [ ! -f ${CPPP} ]; then
	make -C $DIRCPPP
fi 

## Copy makefile
printf "Generating file ${directorioAlumnos}/Makefile \n"
cp $makefileAlumnos ${directorioAlumnos}/Makefile

# Generate source code
for fichero in $ficherosAlumnos
do
	printf "Generating file ${directorioAlumnos}/$fichero \n"
	$CPPP -DALUMNOS $fichero > ${directorioAlumnos}/$fichero
done

echo 'Done!!'
