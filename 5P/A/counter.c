#include <linux/syscalls.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/kifs.h>


int counter;
struct kifs_entry *kifs_entry;

int counter_write(struct kifs_entry* entry, const char *buf, unsigned int len){
	counter++;
	return  len;
}

int counter_read (struct kifs_entry* entry, char *buf, unsigned int len){
	char buffer[10];
	int tam = sprintf(buffer,"%d\n",counter);
	if(copy_to_user(buf, buffer,strlen(buffer))){
		return -EINVAL;
	}
	return tam;
}

struct kifs_operations counter_ops ={
	.read = counter_read,
	.write = counter_write,
};


int __init init_module(void) {
	counter=0;
	kifs_entry = create_kifs_entry("counter",&counter_ops); 
	if(kifs_entry==NULL){
		return -ENOMEM;
	}
	return 0;
}
	
	
void __exit cleanup_module(void)
{   
	remove_kifs_entry("counter");
}
