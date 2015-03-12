#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/kifs.h>
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
static struct kifs_entry *proc_entrym;


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
			
int modlist_write(struct kifs_entry* entry, const char *buf, unsigned int len) {
  	int available_space = BUFFER_LENGTH-1;
	char kbuf[MAX_SIZE];
	int n;

	struct list_head *cur_node=NULL;
	struct list_head *next_node=NULL;
	list_item_t *item=NULL;
  
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
	
	if(sscanf(kbuf,"add %i",&n)==1){
		add_element(n);
	}else if(sscanf(kbuf,"remove %i",&n)==1){
		remove_element(n);
	}else if(strcmp(kbuf,"cleanup")==0){

		list_for_each_safe(cur_node, next_node,&mylist){
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
			
int modlist_read(struct kifs_entry* entry, char *buf, unsigned int len) {
  
	list_item_t *item=NULL;  	
	struct list_head *cur_node=NULL;
	int value;
	char kbuf[MAX_SIZE];
	char *dest=kbuf;
  
	list_for_each(cur_node,&mylist){
		item = list_entry(cur_node,list_item_t,links);
		value = item->data;
		dest+=sprintf(dest,"%i\n",value);
	}


    	/* Transfer data from the kernel to userspace */  
  	if (copy_to_user(buf, kbuf,dest-kbuf))
    		return -EINVAL;

  	return dest-kbuf;
}

struct kifs_operations modlist_ops = {
    	.read = modlist_read,
    	.write = modlist_write,    
};



int init_modlist_module( void )
{
  	int ret = 0;
 	INIT_LIST_HEAD(&mylist);	

	proc_entrym = create_kifs_entry("modlist", &modlist_ops);
    	if (proc_entrym == NULL) {
      			ret = -ENOMEM;
      		printk(KERN_INFO "Modlist: Can't create /proc entry\n");
    	} else {
      		printk(KERN_INFO "Modlist: Module loaded\n");
    	}
  	return ret;
}


void exit_modlist_module( void )
{
  	remove_kifs_entry("modlist");
  	printk(KERN_INFO "Modlist: Module unloaded.\n");
}


module_init( init_modlist_module );
module_exit( exit_modlist_module );
