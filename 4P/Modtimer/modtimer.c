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
#include <linux/uaccess.h>
#include "cbuffer.h"


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module of periodic generation & consumption of numbers concurrently safe - Arquitectura de Linux y Android(UCM)");
MODULE_AUTHOR("Adrian Garcia Garcia");

#define BUFFER_LENGTH       PAGE_SIZE
#define MAX_SIZE  100

static struct proc_dir_entry *proc_entry;
static struct proc_dir_entry *proc_entry_conf;

DEFINE_SPINLOCK(sp);
struct semaphore queue;
struct semaphore mtx;
int nr_waiting;
int transfer_enabled=0;

struct list_head mylist; //Linked list

unsigned long flags;
struct timer_list my_timer;

cbuffer_t* cbuffer;

unsigned int max_random = 300;// Es necesario proteger estas variables?
int emergency_threshold = 80;
int timer_period = 125; // 250 = 1s

struct work_struct my_work;

typedef struct {//Node of the list
	unsigned int data;
	struct list_head links;
}list_item_t;

int add_element(unsigned int operand)
{
	list_item_t *elem = vmalloc(sizeof(list_item_t));
	elem->data = operand;

	down(&mtx);
	list_add_tail(&(elem->links),&mylist);
	up(&mtx);

	return 0;
}

void fire_timer(unsigned long data)
{
	int ncpu = 0;
	int size = 0;
	unsigned int random = get_random_int()%max_random;//si supera a max_random el mod lo corrige,vale??
	printk(KERN_INFO "Inserting in cbuffer(speed:%i timerp) %i\n",timer_period,random);
	spin_lock_irqsave(&sp,flags);

	insert_items_cbuffer_t(cbuffer,(char *)&random,sizeof(unsigned int));
	size = size_cbuffer_t(cbuffer)/sizeof(unsigned int);

	spin_unlock_irqrestore(&sp,flags);

	if((size*10 > emergency_threshold) && !transfer_enabled){
		transfer_enabled=1;
		ncpu=smp_processor_id();

		if(ncpu==1)
			ncpu = 0;
		else
			ncpu = 1;

		schedule_work_on(ncpu,&my_work);
		printk(KERN_INFO "Waking up deferred task (%i items)\n",size);
	}
	mod_timer( &(my_timer), jiffies + timer_period);
}

/* Work's handler function */
static void copy_items_into_list( struct work_struct *work )
{
	int i = 0;
	int j = 0;
	unsigned int* elems = vmalloc(10*sizeof(unsigned int));
	
	spin_lock_irqsave(&sp,flags);
	while(!is_empty_cbuffer_t(cbuffer)){

		remove_items_cbuffer_t(cbuffer,(char *)&elems[i],4);
		i++;

	}
	spin_unlock_irqrestore(&sp,flags);

	for(j = 0; j<i; j++)
		add_element(elems[j]);	
	
	vfree(elems);

	down(&mtx);
	if(nr_waiting > 0){
		up(&queue);
		nr_waiting--;		
	}
	
	up(&mtx);
	transfer_enabled=0;
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
  
	if(down_interruptible(&mtx))
		return -EINTR;

	while(list_empty(&mylist)){
	
		nr_waiting++;
		up(&mtx);

		if(down_interruptible(&queue)){
			down(&mtx);
			nr_waiting--;
			up(&mtx);
			return -EINTR;
		}

		if(down_interruptible(&mtx))
			return -EINTR;
	}

	list_for_each_safe(cur_node, next_node,&mylist){
		
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

    	/* Transfer data from the kernel to userspace */  
  	if (copy_to_user(buf, kbuf,dest-kbuf))
    		return -EINVAL;
    
  	(*off)+=(dest-kbuf);  /* Update the file pointer */

  	return dest-kbuf; // Al haber movido dest de a direccion inicial de kbuf, al restarlos 				  
			  // obtienes los bytes que te has desplazado	
}

static int modtimer_open(struct inode *inode, struct file *file)
{
	try_module_get(THIS_MODULE);

	my_timer.expires = jiffies + timer_period;
	my_timer.function = fire_timer;
	my_timer.data = 0;
	add_timer(&my_timer);

	return 0;
}

static int modtimer_release(struct inode *inode, struct file *file)
{
	struct list_head *cur_node=NULL;
	struct list_head *next_node=NULL;
	list_item_t *item=NULL;

	del_timer_sync(&my_timer);
  	flush_scheduled_work();

	cbuffer->size = 0;

	if(down_interruptible(&mtx))
		return -EINTR;
	list_for_each_safe(cur_node, next_node,&mylist){ // Direccion vale??
		if(!list_empty(&mylist)){
	
			item = list_entry(cur_node,list_item_t,links);
			list_del_init(cur_node);				
			vfree(item);

		}
	}
	up(&mtx);

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
	
	INIT_WORK(&my_work, copy_items_into_list);
	init_timer(&my_timer);
	INIT_LIST_HEAD(&mylist);

	cbuffer = create_cbuffer_t(40);
	if (!cbuffer)
		return -ENOMEM;

	sema_init(&mtx,1);
	sema_init(&queue,0);

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
	destroy_cbuffer_t(cbuffer);

	remove_proc_entry("modtimer", NULL);
	printk(KERN_INFO "Modtimer: Module unloaded.\n");

	remove_proc_entry("modconfig", NULL);
  	printk(KERN_INFO "Modconfig: Module unloaded.\n");
}


module_init( init_modtimer_module );
module_exit( exit_modtimer_module );
