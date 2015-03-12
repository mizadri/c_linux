#include <getopt.h>
#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <err.h>
#include <errno.h>
#include <sys/syscall.h>

//Debian x86_64
//#define SYS_kifs 316
//Android x86_i386
#define SYS_kifs 351

#define USER_BUFFER_MAX 1000
#define MAX_MESSAGE_SIZE 32

enum
{
	KIFS_READ_OP=0,
	KIFS_WRITE_OP=1
};

struct fifo_message {
	unsigned int nr_bytes;
	char data[MAX_MESSAGE_SIZE];
};

void usage(char *argv[],int status)
{
	if(status != EXIT_SUCCESS){
		printf("Usage: %s -p <test-number> [options]\n",argv[0]);
		printf("\nList of options:\n");
		printf("\t-h: Displays this help message\n");
		printf("\t-i <file>: Required to specify input file of pr3\n");
		printf("\t-o: Tests the optional part of pr4\n");
		printf("\t-e <timer-period>: Changes timer-period while testing pr4\n");
	}else{
		fprintf(stderr,"Usage: %s -p <test-number> [options]\n",argv[0]);
		fprintf(stderr,"\nList of options:\n");
		fprintf(stderr,"\t-h: Displays this help message\n");
		fprintf(stderr,"\t-i <file>: Specify input file of pr3\n");
		fprintf(stderr,"\t-o: Tests the optional part of pr4\n");
		fprintf(stderr,"\t-e <timer-period>: Changes timer-period while testing pr4\n");
	}
	exit(status);
}

/* Wrapper to invoke sys_kifs() via syscall() */
int kifs(const char* entry_name,unsigned int op_mode, char* user_buffer,unsigned int maxchars)
{
	return syscall(SYS_kifs,entry_name,op_mode,user_buffer,maxchars); 
}

void testpr4(int optional)
{
	int fdx;
	pid_t pidx;
	int fd;
	int nread = 0;
	char *buf = NULL;
	pid_t pid;
	int b_read = 0;
	
	if(!optional){
/*--------------------------test p4-------------------------*/
		if ( (pid=fork()) == -1) {
			printf("ERROR. Process %u could not fork. Exit\n",pid);
			exit(-1);
		}
		else if (pid==0) {
			//Child pr4 read
				printf("*********Test pr4*********\n");
				printf("Inicio de test con configuracion por defecto\n");
				sleep(1);
				while(nread < 60){
					fd = open("/proc/modtimer", O_RDONLY);
					buf = malloc(50);
					b_read = read(fd, buf, 50);
					buf[b_read] = '\0';				
					printf("%s\n",buf);
					nread++;
					free(buf);
					close(fd);
				}

		
		}
		else {
			// Parent pr4 conf
			sleep(10);//Waits 10s
			
			fd = open("/proc/modconfig", O_WRONLY);

			if(fd > 0){
				write(fd, "emergency_threshold 10\n", 23);	
			}
			else{
				printf("error\n");
				exit(-1);
			}
			close(fd);
			fd = open("/proc/modconfig", O_WRONLY);
	
			if(fd > 0){
				write(fd, "max_random 100\n", 15);
			}
			else{
				printf("error\n");
				exit(-1);
			}
			close(fd);
			fd = open("/proc/modconfig", O_WRONLY);
	
			if(fd > 0){
				write(fd, "timer_period 25\n", 16);	
			}
			else{
				printf("error\n");
				exit(-1);
			}
			printf("Inicio de test con configuracion intensiva\n");
			sleep(1);
			close(fd);
		}


		while (wait(NULL) != -1) ;
			if (errno != ECHILD) {
				printf("ERROR when waiting for childs to finish\n");
				exit(-1);
			}
/*--------------------------test p4-------------------------*/
	}else{//pr4 opc
/*--------------------------test p4 opcional-------------------------*/
		if ( (pidx=fork()) == -1) {
			printf("ERROR. Process %u could not fork. Exit\n",pidx);
			exit(-1);
		}
		else if (pidx==0) {
			//Child pr4 opc
			printf("*********Test pr4 opcional*********\n");
			printf("Inicio de test con configuracion por defecto\n");
			sleep(1);
			if ( (pid=fork()) == -1) {
				printf("ERROR. Process %u could not fork. Exit\n",pid);
				exit(-1);
			}
			else if (pid==0) {
					//Child 1 opc
				sleep(1);
				fd = open("/proc/modtimer", O_RDONLY);
				while(1){
					
					buf = malloc(50);
					b_read = read(fd, buf, 50);
					buf[b_read] = '\0';
					printf("-Impares-\n");					
					printf("%s\n",buf);
					free(buf);
					
				}
				close(fd);
			}
			else {
				//Child 2 opc
				fd = open("/proc/modtimer", O_RDONLY);
				while(1){
					
					buf = malloc(50);
					b_read = read(fd, buf, 50);
					buf[b_read] = '\0';
					printf("-Pares-\n");					
					printf("%s\n",buf);							
					free(buf);
					
				}
				close(fd);
			}


			while (wait(NULL) != -1) ;
				if (errno != ECHILD) {
					printf("ERROR when waiting for childs to finish\n");
					exit(-1);
				}

			exit(0);
		}
		else {
			// Parent pr4 opc
			sleep(10);//Waits 10s
			fdx = open("/proc/modconfig", O_WRONLY);

			if(fdx > 0){
				write(fdx, "emergency_threshold 10\n", 23);	
			}
			else{
				printf("error\n");
				exit(-1);
			}
			close(fdx);
			fdx = open("/proc/modconfig", O_WRONLY);
	
			if(fdx > 0){
				write(fdx, "max_random 10\n", 14);
			}
			else{
				printf("error\n");
				exit(-1);
			}
			close(fdx);
			fdx = open("/proc/modconfig", O_WRONLY);
	
			if(fdx > 0){
				write(fdx, "timer_period 50\n", 16);	
			}
			else{
				printf("error\n");
				exit(-1);
			}
			printf("Inicio de test con configuracion intensiva\n");
			sleep(1);
			close(fdx);
		}


		while (wait(NULL) != -1);
			if (errno != ECHILD) {
				printf("ERROR when waiting for childs to finish\n");
				exit(-1);
			}

		exit(0);
	}
/*--------------------------test p4 opcional-------------------------*/
}

void testpr3(int input_fd)
{
	pid_t pid;
	struct fifo_message message;
	int fd_fifo = 0;
	int bytes=0,wbytes=0;
	const int size=sizeof(struct fifo_message);

	printf("*********Test pr3*********\n");
	sleep(1);
	
	if ( (pid=fork()) == -1) {
		printf("ERROR. Process %u could not fork. Exit\n",pid);
		exit(-1);
	}
	else if (pid==0) {//Child p3(read side)
		fd_fifo=open("/proc/fifoproc",O_WRONLY);

		if (fd_fifo<0) {
			printf("error\n");
			exit(-1);
		}

		/* Bucle de envío de datos a través del FIFO
		- Lee del fichero input
		*/
		while((bytes=read(input_fd,message.data,MAX_MESSAGE_SIZE))>0) {
			message.nr_bytes=bytes;
			wbytes=write(fd_fifo,&message,size);

			if (wbytes > 0 && wbytes!=size) {
				fprintf(stderr,"Can't write the whole register\n");
				exit(1);
			}else if (wbytes < 0){
				perror("Error when writing to the FIFO\n");
				exit(1);
			}		
		}

		if (bytes < 0) {
			fprintf(stderr,"Error when reading from stdin\n");
			exit(1);
		}

		close(fd_fifo);
	}
	else {//Parent p3(write side)
		struct fifo_message message;
		int fd_fifo=0;
		int bytes=0,wbytes=0;
		const int size=sizeof(struct fifo_message);

		fd_fifo=open("/proc/fifoproc",O_RDONLY);

		if (fd_fifo<0) {
			printf("error\n");
			exit(-1);
		}

		while((bytes=read(fd_fifo,&message,size))==size) {
			/* Write to stdout */
			wbytes=write(1,message.data,message.nr_bytes);

			if (wbytes!=message.nr_bytes) {
				fprintf(stderr,"Can't write data to stdout\n");
				exit(1);
			}	
		}

		if (bytes > 0){
			fprintf(stderr,"Can't read the whole register\n");
			exit(1);
		}else if (bytes < 0) {
			fprintf(stderr,"Error when reading from the FIFO\n");
			exit(1);
		}

		close(fd_fifo);
	}

	while (wait(NULL) != -1) ;
		if (errno != ECHILD) {
			printf("ERROR when waiting for childs to finish\n");
			exit(-1);
		}
}

void testpr2(int optional)
{
	pid_t pid;
	int ret;
 	char buff[USER_BUFFER_MAX+1]="";
	int nchars=0;
	int i;
	char addi[8];
	char removei[10];
	int b_written = 0;
	
	if(!optional){
		printf("*********Test pr2*********\n");
		printf("Start test: counter, list\n");
		sleep(1);
		
		if ( (pid=fork()) == -1) {
			printf("ERROR. Process %u could not fork. Exit\n",pid);
			exit(-1);
		}
		else if (pid==0) {//Child p2
			for(i = 0; i< 50; i++){
				ret=kifs("counter",KIFS_WRITE_OP,"count",5);
				/* The return value should be the number of characters read/written.
					If this is not the case, report an error */
				if (ret<0)
				{
					fprintf(stderr,"Error when writing to entry: '%s' (return value: %d)\n","counter",ret);
					perror("Error message");	
					exit(3);
				}	
			}
				
			nchars=USER_BUFFER_MAX;
			ret=kifs("counter",KIFS_READ_OP,buff,nchars);
			
			/* The return value should be the number of characters read/written.
				If this is not the case, report an error */
			if (ret<0)
			{
				fprintf(stderr,"Error when reading entry: '%s' (return value: %d)\n","counter",ret);
				perror("Error message");	
				exit(3);
			}	
			printf("*** READING %s ENTRY **\n","counter");
			buff[ret]='\0';	
			printf("%s\n",buff);
			printf("*****\n");	
		}
		else {//Parent p2
		
			nchars=USER_BUFFER_MAX;
			ret=kifs("list",KIFS_READ_OP,buff,nchars);

			/* The return value should be the number of characters read/written.
			  If this is not the case, report an error */
			if (ret<0)
			{
			  fprintf(stderr,"Error when reading entry: '%s' (return value: %d)\n","list",ret);
			  perror("Error message");	
			  exit(3);
			}	
			printf("*** READING %s ENTRY **\n","list");
			buff[ret]='\0';	
			printf("%s\n",buff);
			printf("*****\n");
		}

		while (wait(NULL) != -1) ;
			if (errno != ECHILD) {
				printf("ERROR when waiting for childs to finish\n");
				exit(-1);
			}
	}else{
		printf("*********Test pr2 optional: modlist*********\n");
		printf("Inserting 20 numbers, then the first 20 first even ones again\n");
		sleep(2);
		//Insert numbers from 1 to 20
		for(i = 0; i<20; i++){
			b_written = sprintf(addi, "add %i",i);
			ret=kifs("modlist",KIFS_WRITE_OP, addi, b_written);
			/* The return value should be the number of characters read/written.
				If this is not the case, report an error */
			if (ret<0)
			{
				fprintf(stderr,"Error when writing to entry: '%s' (return value: %d)\n","modlist",ret);
				perror("Error message");	
				exit(3);
			}	
		}
		
		//Insert even numbers from 1 to 20
		for(i = 0; i<10; i++){
			b_written = sprintf(addi, "add %i",2*i);
			ret=kifs("modlist",KIFS_WRITE_OP, addi, b_written);
			/* The return value should be the number of characters read/written.
				If this is not the case, report an error */
			if (ret<0)
			{
				fprintf(stderr,"Error when writing to entry: '%s' (return value: %d)\n","modlist",ret);
				perror("Error message");	
				exit(3);
			}	
		}
		
		//Print list
		nchars=USER_BUFFER_MAX;
		ret=kifs("modlist",KIFS_READ_OP,buff,nchars);
		
		/* The return value should be the number of characters read/written.
			If this is not the case, report an error */
		if (ret<0)
		{
			fprintf(stderr,"Error when reading entry: '%s' (return value: %d)\n","modlist",ret);
			perror("Error message");	
			exit(3);
		}	
		printf("*** READING %s ENTRY **\n","modlist");
		buff[ret]='\0';	
		printf("%s\n",buff);
		printf("*****\n");
		
		sleep(2);
		
		printf("Removing even numbers from list\n");
		
		sleep(2);
		
		//Remove even numbers from 1 to 20
		for(i = 0; i<10; i++){
			b_written = sprintf(removei, "remove %i",2*i);
			ret=kifs("modlist",KIFS_WRITE_OP, removei, b_written);
			/* The return value should be the number of characters read/written.
				If this is not the case, report an error */
			if (ret<0)
			{
				fprintf(stderr,"Error when writing to entry: '%s' (return value: %d)\n","modlist",ret);
				perror("Error message");	
				exit(3);
			}	
		}
		
		//Print list
		nchars=USER_BUFFER_MAX;
		ret=kifs("modlist",KIFS_READ_OP,buff,nchars);
		
		/* The return value should be the number of characters read/written.
			If this is not the case, report an error */
		if (ret<0)
		{
			fprintf(stderr,"Error when reading entry: '%s' (return value: %d)\n","modlist",ret);
			perror("Error message");	
			exit(3);
		}	
		printf("*** READING %s ENTRY **\n","modlist");
		buff[ret]='\0';	
		printf("%s\n",buff);
		printf("*****\n");
		
		
	}
}

void testpr1(int optional){
	
	int fdx;
	int fd;
	int nread = 0, i = 0;
	char *buf = NULL;
	char removei[9];
	char addi[6];
	int n_o = 0;
	char entero[9]= "123456789"; 
	char c = '\0';
	char *mod_buf = NULL;
	int b_read = 0;
	
	fd = open("/proc/modconfig", O_WRONLY);

	if(fd > 0){
		write(fd, "emergency_threshold 10\n", 23);	
	}
	else{
		printf("error\n");
		exit(-1);
	}
	close(fd);
	fd = open("/proc/modconfig", O_WRONLY);

	if(fd > 0){
		write(fd, "max_random 10\n", 14);
	}
	else{
		printf("error\n");
		exit(-1);
	}
	close(fd);
	fd = open("/proc/modconfig", O_WRONLY);

	if(fd > 0){
		write(fd, "timer_period 190\n", 17);	
	}
	else{
		printf("error\n");
		exit(-1);
	}
	close(fd);
	printf("*********Test pr1 with pr4*********\n");
	printf("Removing and adding numbers got from timer and cleanup(sort with -o) \n");
	sleep(1);
	fd = open("/proc/modtimer", O_RDONLY);
	while(nread < 20){
		
		sprintf(entero,"12345678");
		c = '\0';
		i = 0;
		/*init parsing parameters*/
		mod_buf = malloc(100);
		buf = malloc(50);
		read(fd, buf, 50);					
		nread++;
	
		while( c != '\n' ){
			entero[i] = buf[i];
			i++;
			c = buf[i];
		}
		entero[i] = '\0';//Parsea numeros de hasta 8 cifras
		//Copia los bytes del buffer que devuelve timer hasta
		//que termina de copiar el primer entero(se encuentra \n)
		
		if(nread == 10){
			printf("-cleanup-\n");
			fdx = open("/proc/modlist", O_WRONLY);
			write(fdx, "cleanup\n", 8);
			close(fdx);
		}
		
		fdx = open("/proc/modlist", O_WRONLY);
		if(nread%2 == 1){
			n_o = sprintf(addi, "add %i\n",atoi(entero));
			write(fdx, addi, n_o);
			sleep(1);
			printf("Timer -> add %i\n", atoi(entero));
		
		}else{
			n_o = sprintf(removei, "remove %i\n",atoi(entero));
			write(fdx, removei, n_o);
			sleep(1);
			printf("Timer -> remove %i\n", atoi(entero));
		}
		
		close(fdx);
		
		if(optional && nread == 20){
			printf("-sort-\n");
			fdx = open("/proc/modlist", O_WRONLY);
			write(fdx, "sort\n", 5);
			close(fdx);
		}
		
		
		fdx = open("/proc/modlist", O_RDONLY);
		b_read = read(fdx, mod_buf, 100);
		mod_buf[b_read] = '\0';
		printf("Reading modlist:\n");
		printf("%s\n",mod_buf);
		close(fdx);
		
		
		
		free(mod_buf);
		free(buf);
		
	}
	close(fd);
}

int main(int argc, char *argv[])
{
	int pr = 0, optional = 0;
	char optc;
	int input_fd = 0;

	while ((optc = getopt(argc, argv, "+hp:oe::i:")) != -1) {
		switch (optc) {
			case 'h': 
				usage(argv, EXIT_SUCCESS);
				exit(0);
				break;
				
			case 'p':
				pr = atoi(optarg);  
				break;
				
			case 'o':
			//Test the optional part(pr2 and pr4)
				optional = 1;
				break;
				
			case 'i':
			//Specify input file to pr3
				input_fd = open(optarg,O_RDONLY);
				if (input_fd<0)
					usage(argv, EXIT_FAILURE);
				break;
				
			default:
				fprintf(stderr, "Opcion incorrecta: %c\n", optc);
				usage(argv, EXIT_FAILURE);
		}
	}
        
	switch(pr) {
		case 1:
			testpr1(optional);
			break;
			
		case 2:
			testpr2(optional);
			break;
		
		case 3:
			testpr3(input_fd);
			break;
			
		case 4:
			testpr4(optional);
			break;
			
		default:
			fprintf(stderr, "Practica incorrecta: %c\n", optc);
			usage(argv, EXIT_FAILURE);
			
	}
	
	return 0;
}
