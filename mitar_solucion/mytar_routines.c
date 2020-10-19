#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mytar.h"

extern char *use;

/** Copy nBytes bytes from the origin file to the destination file.
 *
 * origin: pointer to the FILE descriptor associated with the origin file
 * destination:  pointer to the FILE descriptor associated with the destination file
 * nBytes: number of bytes to copy
 *
 * Returns the number of bytes actually copied or -1 if an error occured.
 */
int
copynFile(FILE * origin, FILE * destination, int nBytes)
{
#ifdef ALUMNOS
	// Complete the function
	return -1;
#else
	int nCopy = 0;
	int c;
	while (nCopy < nBytes && (c = getc(origin)) != EOF) {
		putc((unsigned char) c, destination);
		nCopy++;
	}
	return (nCopy);
#endif
}

/** Loads a string from a file.
 *
 * file: pointer to the FILE descriptor 
 * 
 * The loadstr() function must allocate memory from the heap to store 
 * the contents of the string read from the FILE. 
 * Once the string has been properly built in memory, the function returns
 * the starting address of the string (pointer returned by malloc()) 
 * 
 * Returns: !=NULL if success, NULL if error
 */
char*
loadstr(FILE * file)
{
#ifdef ALUMNOS
	// Complete the function
	return NULL;
#else
	int n, size = 0;
	char* buf;

	do {
		n = getc(file);
		size++;
	} while ((n != (int) '\0')
		 && (n != EOF));

	if (n == EOF)
		return NULL;

	if ((buf = (char *) malloc(size)) == NULL)
		return NULL;

	fseek(file, -size, SEEK_CUR);

	fread(buf, 1, size, file);

	return buf;
#endif
}

/** Read tarball header and store it in memory.
 *
 * tarFile: pointer to the tarball's FILE descriptor 
 * nFiles: output parameter. Used to return the number 
 * of files stored in the tarball archive (first 4 bytes of the header).
 *
 * On success it returns the starting memory address of an array that stores 
 * the (name,size) pairs read from the tar file. Upon failure, the function returns NULL.
 */
stHeaderEntry*
readHeader(FILE * tarFile, int *nFiles)
{
#ifdef ALUMNOS
	// Complete the function
	return NULL;
#else
	int i,j;
	stHeaderEntry *header;

	// #files
	fread(nFiles, sizeof (int), 1, tarFile);

	// head of tar
	if ((header = (stHeaderEntry *) malloc(sizeof (stHeaderEntry) * (*nFiles)))
	    == NULL) {
		perror
		    ("Error in memory allocation for the header's file mtar");
		fclose(tarFile);
		return NULL;
	}

	//Load header's file
	for (i = 0; i < *nFiles; i++) {
		if ((header[i].name=loadstr(tarFile))==NULL) {
			for (j = 0; j < *nFiles; j++)
				free(header[j].name);
			free(header);
			fclose(tarFile);
			return NULL;
		}
		fread(&header[i].size, sizeof(header[i].size), 1, tarFile);
	}

	return header;
#endif
}

/** Creates a tarball archive 
 *
 * nfiles: number of files to be stored in the tarball
 * filenames: array with the path names of the files to be included in the tarball
 * tarname: name of the tarball archive
 * 
 * On success, it returns EXIT_SUCCESS; upon error it returns EXIT_FAILURE. 
 * (macros defined in stdlib.h).
 *
 * HINTS: First reserve room in the file to store the tarball header.
 * Move the file's position indicator to the data section (skip the header)
 * and dump the contents of the source files (one by one) in the tarball archive. 
 * At the same time, build the representation of the tarball header in memory.
 * Finally, rewind the file's position indicator, write the number of files as well as 
 * the (file name,file size) pairs in the tar archive.
 *
 * Important reminder: to calculate the room needed for the header, a simple sizeof 
 * of stHeaderEntry will not work. Bear in mind that, on disk, file names found in (name,size) 
 * pairs occupy strlen(name)+1 bytes.
 *
 */
int
createTar(int nFiles, char *fileNames[], char tarName[])
{
#ifdef ALUMNOS
	// Complete the function
	return EXIT_FAILURE;
#else
	int i,j;
	FILE *tarFile, *inputFile;
	stHeaderEntry *header;
	unsigned int headersize;

	//file[s]
	if (nFiles <= 0) {
		fprintf(stderr,"%s",use);
		return (EXIT_FAILURE);
	}
	//Open destination file
	if ((tarFile = fopen(tarName, "wx")) == NULL) {
		fprintf(stderr, "The mtar file %s could not be opened: ",
			tarName);
		perror(NULL);
		return (EXIT_FAILURE);
	}
	//Memory reservation for the header structure of mtar file[s]
	if ((header = malloc(sizeof (stHeaderEntry) * nFiles)) == NULL) {
		perror
		    ("Error at memory allocation in mtar file header");
		fclose(tarFile);
		remove(tarName);
		return (EXIT_FAILURE);
	}
	//Compute #bytes needed for the header and
	//fill the header with the file name[s]
	headersize = sizeof (int);
	for (i = 0; i < nFiles; i++) {
		int namesize = strlen(fileNames[i]) + 1;

		header[i].name = (char *) malloc(namesize);
		if (header[i].name == NULL) {
			perror
			    ("Error at memory allocation for the file name");
			fclose(tarFile);
			remove(tarName);
			for (j = 0; j < i; j++)
				free(header[j].name);
			free(header);
			return (EXIT_FAILURE);
		}
		strcpy(header[i].name, fileNames[i]);

		headersize += namesize + sizeof (header->size);
	}
	//Seek in mtar data area
	fseek(tarFile, headersize, SEEK_SET);

	//Fill header info (at RAM memory) and copy file[s] data into tar
	for (i = 0; i < nFiles; i++) {
		//Source file[s] open
		if ((inputFile = fopen(fileNames[i], "r")) == NULL) {
			fprintf(stderr,
				"Not possible to open tar file %s: \n",
				fileNames[i]);
			perror(NULL);
			fclose(tarFile);
			remove(tarName);
			for (j = 0; j < nFiles; j++)
				free(header[j].name);
			free(header);
			return (EXIT_FAILURE);
		}
		//File[s] copy
		header[i].size = copynFile(inputFile, tarFile, INT_MAX);
		fclose(inputFile);
	}

	//Write each header file[s] with the file[s]
	rewind(tarFile);
	fwrite(&nFiles, sizeof (int), 1, tarFile);
	for (i = 0; i < nFiles; i++) {
		fwrite(header[i].name, 1, strlen(header[i].name) + 1, tarFile);
		fwrite(&header[i].size, sizeof (header[i].size), 1, tarFile);
	}

	fprintf(stdout, "mtar file created successfully\n");

	for (j = 0; j < nFiles; j++)
		free(header[j].name);
	free(header);
	fclose(tarFile);

	return (EXIT_SUCCESS);
#endif
}

/** Extract files stored in a tarball archive
 *
 * tarName: tarball's pathname
 *
 * On success, it returns EXIT_SUCCESS; upon error it returns EXIT_FAILURE. 
 * (macros defined in stdlib.h).
 *
 * HINTS: First load the tarball's header into memory.
 * After reading the header, the file position indicator will be located at the 
 * tarball's data section. By using information from the 
 * header --number of files and (file name, file size) pairs--, extract files 
 * stored in the data section of the tarball.
 *
 */
int
extractTar(char tarName[])
{
#ifdef ALUMNOS
	// Complete the function
	return EXIT_FAILURE;
#else
	stHeaderEntry *header;
	int nFiles, i,j;
	FILE *tarFile, *outputFile;

	//Open destination file
	if ((tarFile = fopen(tarName, "r")) == NULL) {
		fprintf(stderr, "The mtar file %s could not be opened: ",
			tarName);
		perror(NULL);
		return (EXIT_FAILURE);
	}
	//Read header of mtar file
	if ((header=readHeader(tarFile,&nFiles)) == NULL) {
		fclose(tarFile);
		return (EXIT_FAILURE);
	}
	//Create the file[s] included in mtar
	for (i = 0; i < nFiles; i++) {
		if ((outputFile = fopen(header[i].name, "wx")) == NULL) {
			fprintf(stderr,
				"Not possible to create file %s: \n",
				header[i].name);
			perror(NULL);
			for (j = 0; j < nFiles; j++)
				free(header[j].name);
			free(header);
			fclose(tarFile);
			return EXIT_FAILURE;
		}

		printf("[%i]: Creating file %s, size %i Bytes...", i,
		       header[i].name, header[i].size);

		//Copy the file to disk
		if (copynFile(tarFile, outputFile, header[i].size) !=
		    header[i].size) {
			fprintf(stderr,
				"Not possible to copy the file %s: \n",
				header[i].name);
			remove(header[i].name);
			for (j = 0; j < nFiles; j++)
				free(header[j].name);
			free(header);
			fclose(tarFile);
			fclose(outputFile);
			return (EXIT_FAILURE);
		}

		fclose(outputFile);
		printf("Ok\n");

	}

	free(header);
	fclose(tarFile);
	return (EXIT_SUCCESS);
#endif
}
