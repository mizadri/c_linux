#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/kifs.h>
#include <linux/list.h>
#include <linux/uaccess.h>
#include <linux/errno.h>

#define BUFFER_LENGTH       PAGE_SIZE
#define MAX_SIZE  100


struct list_head mylistk; // Linked list of kifs entries
static char *clipboard; 

struct kifs_entry *proc_entryl;
struct kifs_entry *proc_entry;

struct proc_dir_entry *test_dir=NULL;
static struct proc_dir_entry *proc_entryD;


//----------------------------------------P_final-------------------------------------------------
static ssize_t kifs_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
	struct kifs_operations *private_data = (struct kifs_operations *)PDE_DATA(filp->f_inode);
	if(private_data->write != NULL)
		return private_data->write(NULL,buf,len);
	else
		return -EINVAL;
}

static ssize_t kifs_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
	struct kifs_operations *private_data = (struct kifs_operations *)PDE_DATA(filp->f_inode);
	
	if ((*off) > 0) /* Tell the application that there is nothing left to read */
      		return 0;
      			
	if(private_data->read != NULL){
		(*off)+=len;
		return private_data->read(NULL,buf,len);
	}else
		return -EINVAL;
}

static const struct file_operations proc_entry_fops = {
    .read = kifs_read,
    .write = kifs_write,    
};

//----------------------------------------P_final-------------------------------------------------
struct kifs_entry* my_create_kifs_entry(const char* entryname,struct kifs_operations* ops) {
		
	struct kifs_entry *elem=NULL; 
	struct list_head *cur_node=NULL;
	struct list_head *next_node=NULL;
	struct kifs_entry *item=NULL;
	
	if(!list_empty(&mylistk)){
		list_for_each_safe(cur_node, next_node,&mylistk){
			item = list_entry(cur_node,struct kifs_entry,links);
			if(strcmp(item->entryname,entryname) == 0)return NULL;
		}
	}
	
	elem = (struct kifs_entry *)vmalloc(sizeof(struct kifs_entry));
	if(elem==NULL)return NULL;
	else{
		strcpy(elem->entryname,entryname);
		elem->ops = ops;
		elem->private_data=NULL;
		list_add_tail(&(elem->links),&mylistk);
		if((strcmp(entryname,"clipboard")!=0) && (strcmp(entryname,"list")!=0)) try_module_get(THIS_MODULE);
		
		/*-------------------P_final------------------*/
		 /* Create proc directory */ 
		if(!test_dir){
			test_dir=proc_mkdir("kifs", NULL);
			if(!test_dir){
				vfree(elem);
				return NULL;
			}	
		}
		
		proc_entryD = proc_create_data( entryname, 0666, test_dir, &proc_entry_fops, ops);
		
		if(!proc_entryD){
			remove_proc_entry("kifs", NULL);
			vfree(elem);
			return NULL;
		}
		
		/*-------------------P_final------------------*/		
		return elem;
	}
	
	
}

int my_remove_kifs_entry(const char* entry_name) {
	struct list_head *cur_node=NULL;
	struct list_head *next_node=NULL;
	struct kifs_entry *item=NULL;
	int succes = -1;

	list_for_each_safe(cur_node, next_node,&mylistk){
		item = list_entry(cur_node,struct kifs_entry,links);
		if(strcmp(item->entryname,entry_name) == 0){
			list_del_init(cur_node);
			vfree(item);
			if((strcmp(entry_name,"clipboard")!=0) && (strcmp(entry_name,"list")!=0))   module_put(THIS_MODULE);
			remove_proc_entry(entry_name, test_dir);//p-final
			succes = 0;
		}
	}
	return succes;
}

long my_sys_kifs(const char* entry_name, unsigned int op_mode,char* user_buffer, unsigned int maxchars) {


	struct list_head *cur_node=NULL;
	struct list_head *next_node=NULL;
	struct kifs_entry *item=NULL;
	char name[MAX_SIZE_KIFS_ENTRY_NAME];

	strncpy_from_user(name,entry_name,MAX_SIZE_KIFS_ENTRY_NAME-1);

	list_for_each_safe(cur_node, next_node,&mylistk){
		item = list_entry(cur_node,struct kifs_entry,links);
		if(strcmp(name,item->entryname)== 0){

			if((op_mode==1 && item->ops->write == NULL)||(op_mode==0 && item->ops->read == NULL)){	
				return -EINVAL;
			}else{
				if(op_mode==0){
					printk(KERN_INFO "<<Read>>\n");
					return item->ops->read(item,user_buffer,maxchars); 
				}else if(op_mode==1){
					printk(KERN_INFO "<<Write>>\n");
					return item->ops->write(item,user_buffer,maxchars);
				}
			}

		}
	}
	return -EINVAL;// Si llega aqui no ha encontrado ninguna entrada en el for.
}		
//----------------------------------------CLIPBOARD---------------------------------------------
int clipboard_write(struct kifs_entry* entry, const char *buf, unsigned int len) {
  	int available_space = BUFFER_LENGTH-1;
  
  	if (len > available_space) {
   		printk(KERN_INFO "clipboard: not enough space!!\n");
    		return -ENOSPC;
  	}

  	/* Transfer data from user to kernel space */
  	if (copy_from_user( &clipboard[0], buf, len ))  
    	return -EFAULT;

 	clipboard[len] = '\0'; /* Add the `\0' */
  
  	return len;
}

int clipboard_read(struct kifs_entry* entry, char *buf, unsigned int len) {
  	int nr_bytes;
  	nr_bytes=strlen(clipboard);
    
 	if (len<nr_bytes)
    		return -ENOSPC;
  
   	/* Transfer data from the kernel to userspace */  
  	if (copy_to_user(buf, clipboard,nr_bytes))
    		return -EINVAL;

  	return nr_bytes; 
}
//----------------------------------------CLIPBOARD-----------------------------------------------

//----------------------------------------LIST----------------------------------------------------
int list_read(struct kifs_entry* entry, char *buf, unsigned int len) {
	
	struct kifs_entry *item=NULL;  	
	struct list_head *cur_node=NULL;
	char kbuf[MAX_SIZE];
	char *dest=kbuf;

	list_for_each(cur_node,&mylistk){
		item = list_entry(cur_node,struct kifs_entry,links);
		dest+=sprintf(dest,"%s\n",item->entryname);
	}


    	/* Transfer data from the kernel to userspace */  
  	if (copy_to_user(buf, kbuf,dest-kbuf))
    		return -EINVAL;

  	return dest-kbuf;
}

//----------------------------------------LIST----------------------------------------------------

struct kifs_impl_operations ops = {
	.create_kifs_entry=my_create_kifs_entry,
	.remove_kifs_entry=my_remove_kifs_entry,
	.sys_kifs=my_sys_kifs,
};

struct kifs_operations clipboard_ops = {
    	.read = clipboard_read,
    	.write = clipboard_write,    
};

struct kifs_operations list_ops = {
    	.read = list_read,
    	.write = NULL,    
};

int __init init_module(void) {
	int ret = 0;
	clipboard = (char *)vmalloc( BUFFER_LENGTH );
	INIT_LIST_HEAD(&mylistk);
	register_kifs_implementation(&ops);
	
	
  	if (!clipboard) {
		vfree(clipboard);
		unregister_kifs_implementation(&ops);
    		return -ENOMEM;
  	} else {

	    	memset( clipboard, 0, BUFFER_LENGTH );
	    	proc_entry = my_create_kifs_entry("clipboard", &clipboard_ops);
	    	if (proc_entry == NULL) {
	      		vfree(clipboard);
	      		printk(KERN_INFO "Clipboard: Can't create kifs entry\n");
			unregister_kifs_implementation(&ops);
			return -ENOMEM;
	    	} else {
	      		printk(KERN_INFO "Clipboard: Module loaded\n");
    		}
	}
	
	proc_entryl = my_create_kifs_entry("list", &list_ops);
	if (proc_entryl == NULL) {
		vfree(clipboard);		
		unregister_kifs_implementation(&ops);
      		printk(KERN_INFO "List: Can't create kifs entry\n");
		return -ENOMEM;
    	} else {
      		printk(KERN_INFO "List: Module loaded\n");
    	}

return ret;
}

void __exit cleanup_module(void){
	
	my_remove_kifs_entry("list");
	my_remove_kifs_entry("clipboard");
	remove_proc_entry("kifs", NULL);//P-final
	vfree(clipboard);
  	printk(KERN_INFO "Clipboard,List: Entries unloaded.\n");
	unregister_kifs_implementation(&ops);
	
	
}
