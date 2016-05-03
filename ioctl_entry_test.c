#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>  
#include <sys/types.h>  
#include <sys/stat.h>  
#include <fcntl.h>  
#include <linux/ioctl.h>




#define CDRV_IOC_MAGIC  'a'
#define CDRV_IOC_MAXNR 1
#define ASP_CHGACCDIR _IO(CDRV_IOC_MAGIC,   1)
#define MYDEV_NAME "mycdrv"



  
int main()  
{      
    int fp, ret, orignal_dir;  
    unsigned char charecter[]= "ROHIT123";  
    unsigned char charecter1[]="321TIHOR";
    unsigned char *buf1 = (unsigned char *)malloc(sizeof(charecter)+1);  
    unsigned char *buf2 = (unsigned char *)malloc(sizeof(charecter)+1);  
     
    memset(buf1, 0, sizeof(charecter)+1);  
    memset(buf2, 0, sizeof(charecter)+1);  
    strcpy(buf1, charecter); 
    
    fp = open("/dev/mycdrv0", O_RDWR);  
     
    printf("DATA I AM WRITTING IS :%s\n",buf1);
    ret = write(fp, buf1, sizeof(charecter)-1);    
     

    orignal_dir=ioctl(fp, ASP_CHGACCDIR, 1);   
    printf("Original direction is %d. Now it is 1. \n ", orignal_dir); 

    ret = read(fp, buf2, sizeof(charecter)-1); 
     
    printf("read data Output from the device:%s\n", buf2);

    ret = read(fp,buf2, sizeof(charecter)-1); 

    lseek(fp,8,0); 
    strcpy(buf1, charecter1);
    printf("DATA I AM WRITTING IS :%s\n",buf1);  

    ret = write(fp, buf1, sizeof(charecter)-1);
    
    orignal_dir=ioctl(fp, ASP_CHGACCDIR, 0); 
    printf("Original direction is %d. Now it is 0. \n ", orignal_dir); 
    ret = read(fp, buf2, sizeof(charecter)-1); 
      
    printf("read data:%s\n", buf2);

    close(fp);  
    return 0;  
}  
