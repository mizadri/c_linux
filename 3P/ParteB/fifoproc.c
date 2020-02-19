#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/semaphore.h>
#include "cbuffer.h"

#define MAX_CBUFFER_LEN 50
#define MAX_KBUF MAX_CBUFFER_LEN

cbuffer_t* cbuffer; /* Buffer circular */
struct semaphore mtx;
/* para garantizar Exclusión Mutua */
struct semaphore sem_prod; /* cola de espera para productor(es) */
struct semaphore sem_cons; /* cola de espera para consumidor(es) */
int nr_prod_waiting=0; /* Número de procesos productores esperando */
int nr_cons_waiting=0; /* Número de procesos consumidores esperando */
int prod_count = 0;
/* Número de procesos que abrieron la entrada
/proc para escritura (productores) */
int cons_count = 0;
/* Número de procesos que abrieron la entrada
/proc para lectura (consumidores) */
static struct proc_dir_entry *proc_entry;

static int fifoproc_open(struct inode *inode, struct file *file){
	
	if(down_interruptible(&mtx))
		return -EINTR;

	if (file->f_mode & FMODE_READ){/* Un consumidor abrió el FIFO */

		cons_count++;
	
		/*2.Abrir fifo como consumidor sin productor bloquea al proceso*/
		while(prod_count == 0){
			//cond_wait(cons,mtx)
			nr_cons_waiting++;
			up(&mtx);

			if(down_interruptible(&sem_cons)){//sem_cons se inicializa a 0. Se bloquea siempre al llegar.
				down(&mtx);		  //Sale por aqui cuando recibe interrupcion
				nr_cons_waiting--;
				cons_count--;		//No hay que hacerlo en el read/write
				up(&mtx);
				return	-EINTR;		
			}
			if(down_interruptible(&mtx))
				return -EINTR;
			//cond_wait(cons,mtx)
		}
	
		if (nr_prod_waiting>0) {
		/* Despierta a uno de los hilos bloqueados */
			up(&sem_prod);
			nr_prod_waiting--;
		}

		//cond_signal(prod,mtx)
	} else{/* Un productor abrió el FIFO */

		prod_count++;

		/*3.Abrir fifo como productor sin consumidor bloquea al proceso*/
		while(cons_count==0){
			//cond_wait(prod,mtx)
			nr_prod_waiting++;
			up(&mtx);

			if(down_interruptible(&sem_prod)){//sem_prod se inicializa a 0. Se bloquea siempre al llegar.
				down(&mtx);		  //Sale por aqui cuando recibe interrupcion
				nr_prod_waiting--;	 
				prod_count--;		//No hay que hacerlo en el read/write
				up(&mtx);
				return	-EINTR;		
			}

			if(down_interruptible(&mtx))
				return -EINTR;
			//cond_wait(prod,mtx)
		}
	
		if (nr_cons_waiting>0) {
		/* Despierta a uno de los hilos bloqueados */
			up(&sem_cons);
			nr_cons_waiting--;
		}

		//cond_signal(cons,mtx)
	}
	
	up(&mtx);

	return 0;
}

static int fifoproc_release(struct inode *inode, struct file *file){

	if(down_interruptible(&mtx))
		return -EINTR;//6.		

	if (file->f_mode & FMODE_READ){/* Un consumidor cierra el FIFO */
	
		cons_count--;
		if (prod_count>0){
			up(&sem_prod);			
		}	
		
	} else{/* Un productor cierra el FIFO */
		
		prod_count--;
		if (cons_count>0){
			up(&sem_cons);			
		}
	}
	up(&mtx);

	return 0;
}


static ssize_t fifoproc_write(struct file *file, const char *buf, size_t len, loff_t *off){

	char kbuffer[MAX_KBUF];
	
	/*1.lec/esc de mas bytes que tiene el buffer devuelve error*/
	if (len> MAX_CBUFFER_LEN){
		return -ENOSPC;
	}

	if (copy_from_user(kbuffer,buf,len))
		return -EINVAL;
	
	//-------------SC-------------
	if(down_interruptible(&mtx)){
		/*6.Despierta si recibe señal*/
		printk(KERN_INFO "ERROR 136\n");
		return -EINTR;
	}

	if (cons_count==0){
		up(&mtx);
		printk(KERN_INFO "ERROR 142\n");
		return -EPIPE;
	}
	/* 4.		Esperar hasta que haya hueco para insertar (debe haber consumidores) */
	while (nr_gaps_cbuffer_t(cbuffer)<len && cons_count>0){
		//cond_wait(prod,mtx)
		nr_prod_waiting++;
		up(&mtx);

		if(down_interruptible(&sem_prod)){//Se bloquea siempre al llegar(0)
			/*6.Despierta si recibe señal*/
			down(&mtx);
			nr_prod_waiting--;	 
			up(&mtx);
			printk(KERN_INFO "ERROR 156\n");
			return	-EINTR;		
		}

		if(down_interruptible(&mtx))
			return -EINTR;
		//cond_wait(prod,mtx)
	}
	
	if (cons_count==0){
		up(&mtx);
		printk(KERN_INFO "ERROR 167\n");
		return -EPIPE;
	}
	
	insert_items_cbuffer_t(cbuffer,kbuffer,len);
	
	//cond_singal(cons,mtx)
	if (nr_cons_waiting>0) {
	/* Despierta a uno de los hilos bloqueados */
		up(&sem_cons);
		nr_cons_waiting--;
	}
	//cond_signal(cons,mtx)

	up(&mtx);
	//-------------SC-------------
	
	return len;
}


static ssize_t fifoproc_read(struct file *file, char *buf, size_t len, loff_t *off){
	char kbuff[MAX_KBUF];

	/*1.lec/esc de mas bytes que tiene el buffer devuelve error*/
	if (len> MAX_CBUFFER_LEN){
		return -ENOSPC;
	}
	
	//-------------SC-------------
	if (down_interruptible(&mtx)){

		return -EINTR;
	}

	/*8.Si se intenta hacer una lectura cuando esta vacio y no hay productores se devuelve 0*/
	if ((size_cbuffer_t(cbuffer)==0) && (prod_count==0)){
		up(&mtx);	
		return 0;
	}	

	/*5.Bloquearse mientras:tiene bytes de los solicitados por read*/
 	while (size_cbuffer_t(cbuffer)<len&&prod_count>0){
		//cond_wait(cons,mtx)
		/* Incremento de consumidores esperando */
		nr_cons_waiting++;

		/* Liberar el 'mutex' antes de bloqueo*/
		up(&mtx);
	
		if (down_interruptible(&sem_cons)){//Se bloquea siempre al llegar(0)
			down(&mtx);
			nr_cons_waiting--;
			up(&mtx);		
			return -EINTR;
		}	
	
		/* Readquisición del 'mutex' antes de entrar a la SC */		
		if (down_interruptible(&mtx)){		
			return -EINTR;
		}
		//cond_wait(cons,mtx)	
  	}
	
	if((prod_count==0)&&(size_cbuffer_t(cbuffer)==0)){
		up(&mtx);
		return 0;
	}
	
	remove_items_cbuffer_t(cbuffer,kbuff,len);
  	
	//cond_singal(prod,mtx)
	if (nr_prod_waiting>0){
		up(&sem_prod);	
		nr_prod_waiting--;
  	}
	//cond_singal(prod,mtx)

	up(&mtx);
	//-------------SC-------------	

	if (copy_to_user(buf,kbuff,len)){
		return -EINVAL;
	}
	
	return len;
}

static const struct file_operations proc_entry_fops = {
    	.write = fifoproc_write,
	.read = fifoproc_read,
	.release = fifoproc_release,
	.open = fifoproc_open, 
};

int init_module(void){
	int ret=0;
	sema_init(&mtx,1);
	sema_init(&sem_prod,0);
	sema_init(&sem_cons,0);

	cbuffer = create_cbuffer_t(50);
	if (!cbuffer)
		return -ENOMEM;
	  	
	proc_entry = proc_create( "fifoproc", 0666, NULL, &proc_entry_fops);
    	if (proc_entry == NULL) {
		destroy_cbuffer_t(cbuffer);
		ret = -ENOMEM;
		printk(KERN_INFO "fifoproc: Can't create /proc entry\n");
	}
      	
	printk(KERN_INFO "fifoproc: Module loaded.\n");
    	
  	return ret;
}
void cleanup_module(void){
	destroy_cbuffer_t(cbuffer);
	remove_proc_entry("fifoproc", NULL);
  	printk(KERN_INFO "fifoproc: Module unloaded.\n");
}
