#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/random.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <asm-generic/uaccess.h>
#include "cbuffer.h"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module of periodic generation & consumption of numbers concurrently safe. One read for evens and one for odds. - Arquitectura de Linux y Android(UCM)");
MODULE_AUTHOR("Adrian Garcia Garcia");

#define BUFFER_LENGTH       PAGE_SIZE
#define MAX_SIZE  100

static struct proc_dir_entry *proc_entry;
static struct proc_dir_entry *proc_entry_conf;

DEFINE_SPINLOCK(sp);
struct semaphore even_queue;
struct semaphore odd_queue;
struct semaphore mtx;
int nr_even_waiting;
int nr_odd_waiting;

unsigned int max_random = 300;// Es necesario proteger estas variables?
int emergency_threshold = 80;
int timer_period = 75; // 250 = 1s

unsigned long flags;
struct timer_list my_timer;
/* Workqueue descriptor */
static struct workqueue_struct* my_wq;
struct work_struct my_work;

cbuffer_t* cbuffer;
struct list_head my_even_list; //Linked list of even numbers(pares)
struct list_head my_odd_list; //Linked list of odd numbers(impares)

typedef struct {//Node of the list
	unsigned int data;
	struct list_head links;
}list_item_t;

int is_even_list = 0;
int n_proc = 0;
int transfer_disabled=0;

int add_element(unsigned int operand,int is_odd)
{
	struct list_head *my_list=NULL;
	list_item_t *elem = vmalloc(sizeof(list_item_t));
	elem->data = operand;

	down(&mtx);
	if(is_odd)
		my_list = &my_odd_list;		
	else
		my_list = &my_even_list;

	
	list_add_tail(&(elem->links),my_list);
	up(&mtx);

	return 0;
}

void fire_timer(unsigned long data)
{
	int size = 0;
	unsigned int random = get_random_int()%max_random;//si supera a max_random el mod lo corrige,vale??
	printk(KERN_INFO "Inserting in cbuffer(speed:%i): %i\n",timer_period,random);
	spin_lock_irqsave(&sp,flags);

	insert_items_cbuffer_t(cbuffer,(char *)&random,sizeof(unsigned int));
	size = size_cbuffer_t(cbuffer)/sizeof(unsigned int);

	spin_unlock_irqrestore(&sp,flags);

	if((size*10 > emergency_threshold) && !transfer_disabled){
		transfer_disabled=1;
		queue_work( my_wq, &my_work );
	}
	mod_timer( &(my_timer), jiffies + timer_period);
}

/* Work's handler function */
static void copy_items_into_list( struct work_struct *work )
{
	int j = 0;
	int i = 0;
	int n_even = 0;
	int n_odd = 0;
	unsigned int* elems = vmalloc(10*sizeof(unsigned int));

	spin_lock_irqsave(&sp,flags);
	while(!is_empty_cbuffer_t(cbuffer)){

		remove_items_cbuffer_t(cbuffer,(char *)&elems[i],4);
		i++;

	}
	spin_unlock_irqrestore(&sp,flags);

	for(j = 0; j<i; j++){		
		if(elems[j] % 2){
			add_element(elems[j],1);
			n_odd++;
		}else{
			add_element(elems[j],0);
			n_even++;
		}		
	}
	vfree(elems);
	down(&mtx);
	
	if(n_odd){
		if(nr_odd_waiting > 0){
			up(&odd_queue);
			nr_odd_waiting--;		
		}
	}
	if(n_even){
		if(nr_even_waiting > 0){
			up(&even_queue);
			nr_even_waiting--;		
		}
	}

	up(&mtx);
	transfer_disabled=0;
}

static ssize_t modtimer_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{ 
	struct list_head *cur_node=NULL;
	struct list_head *next_node=NULL;
	list_item_t *item=NULL;
	unsigned int value;
	char kbuf[MAX_SIZE];
	char *dest=kbuf;
	char line[10];
	int nchars = 0;
	int acum = 0;
	int is_even = 0;

	if(down_interruptible(&mtx))
		return -EINTR;
	is_even = *((int *)filp->private_data);
	up(&mtx);
  
	if(is_even){
		
		if(down_interruptible(&mtx))
			return -EINTR;
		while(list_empty(&my_even_list)){
	
			nr_even_waiting++;
			up(&mtx);

			if(down_interruptible(&even_queue)){
				down(&mtx);
				nr_even_waiting--;
				up(&mtx);
				return -EINTR;
			}

			if(down_interruptible(&mtx))
				return -EINTR;
		}

		list_for_each_safe(cur_node, next_node,&my_even_list){
		
			item = list_entry(cur_node,list_item_t,links);
			value = item->data;
			list_del_init(cur_node);				
			vfree(item);
			nchars = sprintf(line, "%i\n",value);

			if(nchars + acum > 200)
				break;
			else{
				dest+=sprintf(dest,"%s",line);
				acum+=nchars;		
			}		
		
		}
		up(&mtx);
	}else{
		if(down_interruptible(&mtx))
			return -EINTR;

		while(list_empty(&my_odd_list)){
	
			nr_odd_waiting++;
			up(&mtx);

			if(down_interruptible(&odd_queue)){
				down(&mtx);
				nr_odd_waiting--;
				up(&mtx);
				return -EINTR;
			}

			if(down_interruptible(&mtx))
				return -EINTR;
		}

		list_for_each_safe(cur_node, next_node,&my_odd_list){
		
			item = list_entry(cur_node,list_item_t,links);
			value = item->data;
			list_del_init(cur_node);				
			vfree(item);
			nchars = sprintf(line, "%i\n",value);

			if(nchars + acum > 200)
				break;
			else{
				dest+=sprintf(dest,"%s",line);
				acum+=nchars;		
			}		
		
		}
		up(&mtx);
	}
    	/* Transfer data from the kernel to userspace */  
  	if (copy_to_user(buf, kbuf,dest-kbuf))//Esto es necesario?? En kbuf no guardamos nada
    		return -EINVAL;
    
  	(*off)+=(dest-kbuf);  /* Update the file pointer */

  	return dest-kbuf; // Al haber movido dest de a direccion inicial de kbuf, al restarlos 	
			  // obtienes los bytes que te has desplazado	
}

static int modtimer_open(struct inode *inode, struct file *file)
{	
	int *is_even = vmalloc(sizeof(int));	

	try_module_get(THIS_MODULE);

	if(down_interruptible(&mtx))
		return -EINTR;
	
	if(is_even_list){
		is_even_list = 0;	
		*is_even = 0;
	}else{
		is_even_list = 1;
		*is_even = 1;
	}//is_even_list is 0 at start. So the first proc will be even
	n_proc++;
	file->private_data = is_even;
	
	if(n_proc==2){ // El proceso impar despetará al proceso par que esta esperando
		up(&even_queue);
		nr_even_waiting--;
	}
	
	while(n_proc < 2){

		nr_even_waiting++;
		up(&mtx);

		if(down_interruptible(&even_queue)){
			down(&mtx);
			nr_even_waiting--;
			up(&mtx);
			return -EINTR;
		}

		if(down_interruptible(&mtx))
			return -EINTR;
	}
	up(&mtx);
	
	if(*is_even){
		my_timer.expires = jiffies + timer_period;
		my_timer.function = fire_timer;
		my_timer.data = 0;
		add_timer(&my_timer);
	}

	return 0;
}

static int modtimer_release(struct inode *inode, struct file *file)
{
	struct list_head *cur_node=NULL;
	struct list_head *next_node=NULL;
	list_item_t *item=NULL;
	int is_even = 0;

	if(down_interruptible(&mtx))
		return -EINTR;
	is_even = *((int *)file->private_data);
	up(&mtx);

	del_timer_sync(&my_timer);

  	flush_workqueue( my_wq );
  	
	cbuffer->size = 0;

	if(down_interruptible(&mtx))
		return -EINTR;
	if(is_even){	
		list_for_each_safe(cur_node, next_node,&my_even_list){ // Direccion vale??
			if(!list_empty(&my_even_list)){
	
				item = list_entry(cur_node,list_item_t,links);
				list_del_init(cur_node);				
				vfree(item);

			}
		}
	}else{
		list_for_each_safe(cur_node, next_node,&my_odd_list){ // Direccion vale??
			if(!list_empty(&my_odd_list)){
	
				item = list_entry(cur_node,list_item_t,links);
				list_del_init(cur_node);				
				vfree(item);

			}
		}
	}
	up(&mtx);
	vfree(file->private_data);
	module_put(THIS_MODULE);

	return 0;
}

static ssize_t modconfig_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
	char kbuf[MAX_SIZE];
	char *dest=kbuf;
  
	if ((*off) > 0) /* Tell the application that there is nothing left to read */
      		return 0;
    
	dest+=sprintf(dest,"timer_period=%i\n",timer_period);
	dest+=sprintf(dest,"emergency_threshold=%i\n",emergency_threshold);
	dest+=sprintf(dest,"max_random=%i\n",max_random);

    	/* Transfer data from the kernel to userspace */  
  	if (copy_to_user(buf, kbuf,dest-kbuf))
    		return -EINVAL;
    
  	(*off)+=(dest-kbuf);  /* Update the file pointer */

  	return dest-kbuf; // Al haber movido dest de a direccion inicial de kbuf, al restarlos 				  
			  // obtienes los bytes que te has desplazado	
}

static ssize_t modconfig_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
	char kbuf[MAX_SIZE];
	int n;
	unsigned int nmax;
  
	if ((*off) > 0) /* The application can write in thisentry just once !! */
		return 0;

	if(len > MAX_SIZE){
		printk(KERN_INFO "Not enough space!!\n");
		return -ENOSPC;
	}
	
 	 /* Transfer data from user to kernel space */
  	if (copy_from_user( kbuf, buf, len )) 
    		return -EFAULT;

	kbuf[len]='\0';
	*off+=len;           /*Update the file pointer */

	if(sscanf(kbuf,"max_random %i",&nmax)==1){
		max_random = nmax;
	}else if(sscanf(kbuf,"emergency_threshold %i",&n)==1){
		emergency_threshold = n;
	}else if(sscanf(kbuf,"timer_period %i",&n)==1){
		timer_period = n;
	}	
	else{
		printk(KERN_INFO "Argumentos inválidos: max_random <int> , emergency_threshold <int>, timer_period <int>");
	}
  
  	return len;
}

static const struct file_operations proc_entry_fops = {
	.release = modtimer_release,
	.open = modtimer_open,
    	.read = modtimer_read,
    	.write = NULL,    
};

static const struct file_operations proc_entry_conf_fops = {
    	.read = modconfig_read,
    	.write = modconfig_write,    
};

int init_modtimer_module( void )
{
  	int ret = 0;
	
	my_wq = create_workqueue("my_queue");
	if (!my_wq)
		return -ENOMEM;

	INIT_WORK(&my_work, copy_items_into_list);
	init_timer(&my_timer);
	INIT_LIST_HEAD(&my_even_list);
	INIT_LIST_HEAD(&my_odd_list);
	
	cbuffer = create_cbuffer_t(40);
	if (!cbuffer)
		return -ENOMEM;

	sema_init(&mtx,1);
	sema_init(&odd_queue,0);
	sema_init(&even_queue,0);

	proc_entry = proc_create( "modtimer", 0666, NULL, &proc_entry_fops);
    	if (proc_entry == NULL) {
      		ret = -ENOMEM;
		del_timer_sync(&my_timer);
		destroy_cbuffer_t(cbuffer);				      		
		printk(KERN_INFO "Modtimer: Can't create /proc entry\n");
    	} else {
      		printk(KERN_INFO "Modtimer: Module loaded\n");
    	}

	proc_entry_conf = proc_create( "modconfig", 0666, NULL, &proc_entry_conf_fops);
    	if (proc_entry_conf == NULL) {
      			return -ENOMEM;
      		printk(KERN_INFO "modconfig: Can't create /proc entry\n");
    	} else {
      		printk(KERN_INFO "modconfig: Module loaded\n");
    	}
    	
  	return ret;
}

void exit_modtimer_module( void )
{
	destroy_workqueue( my_wq );
	destroy_cbuffer_t(cbuffer);

	remove_proc_entry("modtimer", NULL);
	printk(KERN_INFO "Modtimer: Module unloaded.\n");

	remove_proc_entry("modconfig", NULL);
  	printk(KERN_INFO "Modconfig: Module unloaded.\n");
}

module_init( init_modtimer_module );
module_exit( exit_modtimer_module );
