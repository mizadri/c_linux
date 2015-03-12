#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/list.h>
#include <asm-generic/uaccess.h>



MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module of linked list management - Arquitectura de Linux y Android(UCM)");
MODULE_AUTHOR("Adrian Garcia Garcia");

#define BUFFER_LENGTH       PAGE_SIZE
#define MAX_SIZE  100

struct list_head mylist; //Linked list

typedef struct {//Node of the list
int data;
struct list_head links;
}list_item_t;


int add_element(int operand){

list_item_t *elem = vmalloc(sizeof(list_item_t));
elem->data = operand;
list_add_tail(&(elem->links),&mylist);

return 0;
}

int remove_element(int operand){
	struct list_head *cur_node=NULL;
	struct list_head *next_node=NULL;
	list_item_t *item=NULL;
	int value;

	list_for_each_safe(cur_node, next_node,&mylist){
		item = list_entry(cur_node,list_item_t,links);
		value = item->data;
		if(value == operand){
			list_del_init(cur_node);
			vfree(item);
		}
	}
	return 0;
}	

static struct proc_dir_entry *proc_entry;

static ssize_t modlist_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
  	int available_space = BUFFER_LENGTH-1;
	char kbuf[MAX_SIZE];
	int n;

	struct list_head *cur_node=NULL;
	struct list_head *next_node=NULL;
	list_item_t *item=NULL;
  
	if ((*off) > 0) /* The application can write in thisentry just once !! */
		return 0;

	if (len > available_space) {
   		printk(KERN_INFO "Not enough space!!\n");
    		return -ENOSPC;
  	}
	if(len > MAX_SIZE){
		printk(KERN_INFO "Not enough space!!\n");
		return -ENOSPC;
	}
	
 	 /* Transfer data from user to kernel space */
  	if (copy_from_user( kbuf, buf, len ))  
    		return -EFAULT;
	kbuf[len]='\0';
	*off+=len;           /*Update the file pointer */
	if(sscanf(kbuf,"add %i",&n)==1){
		add_element(n);
	}else if(sscanf(kbuf,"remove %i",&n)==1){
		remove_element(n);
	}else if(strcmp(kbuf,"cleanup\n")==0){

		list_for_each_safe(cur_node, next_node,&mylist){ // Direccion vale??
			if(!list_empty(&mylist)){
		
				item = list_entry(cur_node,list_item_t,links);
				list_del_init(cur_node);
				vfree(item);
			}
		}
	
	}	
	else{
		printk(KERN_INFO "Argumentos inválidos: add <int> , remove <int>, cleanup, cat");
	}
  
  	return len;
}

static ssize_t modlist_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
  
	list_item_t *item=NULL;  	
	struct list_head *cur_node=NULL;
	int value;
	char kbuf[MAX_SIZE];
	char *dest=kbuf;
  
	if ((*off) > 0) /* Tell the application that there is nothing left to read */
      		return 0;
    
  
	list_for_each(cur_node,&mylist){
		item = list_entry(cur_node,list_item_t,links);
		value = item->data;
		dest+=sprintf(dest,"%i\n",value);
	}


    	/* Transfer data from the kernel to userspace */  
  	if (copy_to_user(buf, kbuf,dest-kbuf))//Esto es necesario?? En kbuf no guardamos nada
    		return -EINVAL;
    
  	(*off)+=(dest-kbuf);  /* Update the file pointer */

  	return dest-kbuf; // Al haber movido dest de a direccion inicial de kbuf, al restarlos 				  
			  // obtienes los bytes que te has desplazado	
}

static const struct file_operations proc_entry_fops = {
    	.read = modlist_read,
    	.write = modlist_write,    
};



int init_modlist_module( void )
{
  	int ret = 0;
 	INIT_LIST_HEAD(&mylist);	

    	proc_entry = proc_create( "modlist", 0666, NULL, &proc_entry_fops);
    	if (proc_entry == NULL) {
      			ret = -ENOMEM;
      		printk(KERN_INFO "Modlist: Can't create /proc entry\n");
    	} else {
      		printk(KERN_INFO "Modlist: Module loaded\n");
    	}
  	return ret;
}


void exit_modlist_module( void )
{
  	remove_proc_entry("modlist", NULL);
  	printk(KERN_INFO "Modlist: Module unloaded.\n");
}


module_init( init_modlist_module );
module_exit( exit_modlist_module );
