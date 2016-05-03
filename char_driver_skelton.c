/*************************************************HEADERS*****************************************************/
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h> 
#include <linux/kernel.h>       
#include <linux/slab.h>         
#include <linux/fs.h>           
#include <linux/errno.h>        
#include <linux/types.h>        
#include <linux/proc_fs.h>
#include <linux/fcntl.h>        
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/semaphore.h> 
#include <asm/uaccess.h>        
#include <linux/list.h>
/***********************************************************************************************************/


#define ramdisk_size (size_t) (16*PAGE_SIZE)
#define CDRV_IOC_MAGIC  'a'
#define CDRV_IOC_MAXNR 1
#define ASP_CHGACCDIR _IO(CDRV_IOC_MAGIC,   1)
#define MYDEV_NAME "mycdrv"



static void my_exit(void);
static struct class *foo_class;
int dev_major=  0;
int dev_minor =  0;
int NUM_DEVICES = 3;
int DIRECTION=0;
struct semaphore sem_d;
module_param(NUM_DEVICES, int, S_IRUSR);

/**********STRUCTURE**************************************************************************************/
struct asp_mycdrv 
{
   struct list_head list;
   struct cdev dev;
   char *ramdisk;
   struct semaphore sem;
   int devNo;
} ;



struct asp_mycdrv *dev_devices;
int ret;
dev_t dev_num;


char __user * reverse(char __user * buf, int n)
  {
    int i;
    char c;
    for (i=0;i<n/2;i++)
    {
      c=buf[i];
      buf[i]=buf[n-1-i];
      buf[n-1-i]=c;
      c=buf[i];
    }
    return buf;   
  }

int device_open(struct inode *inode, struct file *filp)
{
   struct asp_mycdrv *dev; 
   dev = container_of(inode->i_cdev, struct asp_mycdrv, dev);   
   filp->private_data = dev; 
   pr_info(" OPENING device: mycdrv%d:\n\n", dev->devNo);
   return 0;          
}

static ssize_t
device_read(struct file *file, char __user * buf, size_t lbuf, loff_t * ppos)
{
   struct asp_mycdrv *dev = file->private_data;
   int nbytes=0;
   if (down_interruptible(&dev->sem))
      return -ERESTARTSYS;
   if (down_interruptible(&sem_d))
      return -ERESTARTSYS;
   pr_info("\n READING function, direction is: %d\n", DIRECTION);
   
   if(DIRECTION==0)
   {
     if ((lbuf + *ppos) > ramdisk_size) {
        pr_info("trying to read past end of device\n");
        up(&sem_d);
        up(&dev->sem);
        return 0;
     }
     nbytes = lbuf - copy_to_user(buf, dev->ramdisk + *ppos, lbuf);
     *ppos += nbytes;
    }

   else if(DIRECTION==1)
   {
     if ((*ppos -lbuf) < 0) {
        pr_info("trying to read past beginning of device\n");
        up(&sem_d);
        up(&dev->sem);
        return 0;
     }
     nbytes = lbuf - copy_to_user(buf, dev->ramdisk + *ppos-lbuf, lbuf);
     *ppos -=nbytes;
     buf=reverse(buf,lbuf);
    }

    else pr_info("DIRECTION not reconized!\n");   
   
   pr_info("\n READING function, nbytes=%d, pos=%d\n", nbytes, (int)*ppos);
   up(&sem_d);
   up(&dev->sem);
   return nbytes; 
}

static ssize_t
device_write(struct file *file, const char __user * buf, size_t lbuf,
        loff_t * ppos)
{
   int nbytes=0,i;
   char * buffer;
   struct asp_mycdrv *dev = file->private_data;
   mm_segment_t old_fs;

   if (down_interruptible(&dev->sem))
      return -ERESTARTSYS;
   buffer=kmalloc((lbuf)*sizeof(char), GFP_KERNEL );   
   if (down_interruptible(&sem_d))
      return -ERESTARTSYS;
    pr_info("\n WRITING function, direction is: %d\n", DIRECTION);
   if(DIRECTION==0)
   {
     if ((lbuf + *ppos) > ramdisk_size) {
        pr_info("trying to write past end of device\n");
        up(&sem_d);
        up(&dev->sem);
        return 0;
     }
   nbytes = lbuf - copy_from_user(dev->ramdisk + *ppos, buf, lbuf);
   *ppos += nbytes;
    }

   else if(DIRECTION==1)
   {
     if ((*ppos -lbuf) < 0) {
        pr_info("trying to write past beginning of device\n");
        up(&sem_d);
        up(&dev->sem);
        return 0;
     }
     
     for(i=0;i<lbuf;i++)
      buffer[i]=buf[lbuf-1-i];       
     buf=buffer;
     printk("\n WRITING function output, %s \n", buf);      
     old_fs = get_fs();
     set_fs(KERNEL_DS);
     nbytes = lbuf - copy_from_user(dev->ramdisk + *ppos-lbuf, buf, lbuf);
     set_fs(old_fs);
     *ppos -=nbytes;
    }

    else pr_info("DIRECTION not reconized!\n");

   pr_info("\n WRITING function, nbytes=%d, pos=%d\n", nbytes, (int)*ppos);
   kfree(buffer);
   up(&sem_d);
   up(&dev->sem);
   return nbytes;
}

static int device_release(struct inode *inode, struct file *file)
{
   struct asp_mycdrv *dev = file->private_data;
   pr_info(" CLOSING device: mycdrv%d:\n\n", dev->devNo);
   return 0;
}

static loff_t device_lseek(struct file *file, loff_t offset, int orig) 
{
	struct asp_mycdrv *dev = file->private_data;
  loff_t testpos;
  if (down_interruptible(&dev->sem))
      return -ERESTARTSYS;
	switch (orig) {
	case SEEK_SET:
		testpos = offset;
		break;
	case SEEK_CUR:
		testpos = file->f_pos + offset;
		break;
	case SEEK_END:
		testpos = ramdisk_size + offset;
		break;
	default:
		return -EINVAL;
	}
	testpos = testpos < ramdisk_size ? testpos : ramdisk_size;
	testpos = testpos >= 0 ? testpos : 0;
	file->f_pos = testpos;
	pr_info("Seeking to pos=%ld\n", (long)testpos);
  up(&dev->sem);
	return testpos;
}

long device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
   
   int err = 0;
   int retval = 0;
   
  
   if (_IOC_TYPE(cmd) != CDRV_IOC_MAGIC) return -ENOTTY;
   if (_IOC_NR(cmd) > CDRV_IOC_MAXNR) return -ENOTTY;

   if (_IOC_DIR(cmd) & _IOC_READ)
      err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
   else if (_IOC_DIR(cmd) & _IOC_WRITE)
      err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
   if (err) return -EFAULT;
   
   switch(cmd) {

   case ASP_CHGACCDIR:
      if (! capable (CAP_SYS_ADMIN))
        return -EPERM;
      if (down_interruptible(&sem_d))
        return -ERESTARTSYS; 
      retval=DIRECTION;
      DIRECTION=arg;
      up(&sem_d);
      return retval;
         
   default:  
      return -ENOTTY;
   }
 }


struct file_operations fops = {
   .owner = THIS_MODULE,
   .open = device_open,
   .release = device_release,
   .write = device_write,
   .read = device_read,
   .llseek=device_lseek,
   .unlocked_ioctl = device_ioctl
};



static void device_setup_cdev(struct asp_mycdrv *dev, int index) {
   int err, devno = MKDEV(dev_major, dev_minor + index);   
   cdev_init(&dev->dev, &fops);
   dev->dev.owner = THIS_MODULE;
   dev->dev.ops = &fops;
   err = cdev_add (&dev->dev, devno, 1);
   if (err)
      printk(KERN_NOTICE "Error %d adding device%d", err, index);
}


/****************************INIT**********************************************/

static int my_init(void)
{
     int result, i;
     dev_t dev = 0;
     LIST_HEAD(head);
   printk("DEVICE GETTING INITIALIZED:\n");
   
   if (dev_major) {
      dev = MKDEV(dev_major, dev_minor);
      result = register_chrdev_region(dev, NUM_DEVICES, MYDEV_NAME);
   } else {
      result = alloc_chrdev_region(&dev, dev_minor, NUM_DEVICES,
               MYDEV_NAME);
      dev_major = MAJOR(dev);
   }
   if (result < 0) {
      printk(KERN_WARNING "DEVICE CAN'T GET A MAJOR NUMBER %d\n", dev_major);
      return result;
   }
   printk("DEVICE MODULE REGISTERED AND ITS MAJOR NUMBER IS:%d\n", dev_major);
   dev_devices = kmalloc(NUM_DEVICES*sizeof(struct asp_mycdrv), GFP_KERNEL);
   if (!dev_devices) {
      result = -ENOMEM;
      printk("FAILURE:MALLOC FUNC\n");  
   }
   memset(dev_devices, 0, NUM_DEVICES * sizeof(struct asp_mycdrv));
   
   foo_class = class_create(THIS_MODULE, "my_class");
   sema_init(&sem_d, 1);
   for (i = 0; i <  NUM_DEVICES; i++) {
      dev_devices[i].ramdisk = kmalloc(ramdisk_size, GFP_KERNEL);
      dev_devices[i].devNo=i;
      sema_init(&dev_devices[i].sem, 1);
      device_setup_cdev(&dev_devices[i], i);
      device_create(foo_class, NULL, MKDEV(MAJOR(dev), MINOR(dev) + i) , NULL, "mycdrv%d", i);
      list_add (&dev_devices[i].list ,&head) ;
      printk("set up the %dth device.\n",i);
   }
   return 0;
 }

/*****************************************EXIT**************************************************/
static void my_exit(void)
{
   int i;
   dev_t devno = MKDEV(dev_major, dev_minor);
   
   
   if (dev_devices) {
      for (i = 0; i < NUM_DEVICES; i++) {
    kfree(dev_devices[i].ramdisk);
    cdev_del(&dev_devices[i].dev);
    device_destroy(foo_class, MKDEV(MAJOR(devno), MINOR(devno) + i));
      }
      kfree(dev_devices);
      class_destroy(foo_class);
   }
  
   
   unregister_chrdev_region(devno, NUM_DEVICES);
   printk("DEVICE MODULE UNREGISTERED FROM DEV!\n");
}

module_init(my_init);
module_exit(my_exit);
MODULE_AUTHOR("USER");
MODULE_LICENSE("GPL v2"); 
