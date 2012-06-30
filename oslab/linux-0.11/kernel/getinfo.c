#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <utime.h>
#include <sys/stat.h>
#include <string.h>
#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/kernel.h>
#include <asm/segment.h>


int sys_getinfo(void * BIOS)
{
	int i,drive;
	for (drive=0 ; drive<4 ; drive++) {
		//hd_info[drive].cyl = *(unsigned short *) BIOS;
		printk("drive = %d,hd_info[drive].cyl = %d\n",drive,*(unsigned short *) BIOS);
		//hd_info[drive].head = *(unsigned char *) (2+BIOS);
		printk("drive = %d,hd_info[drive].head = %d\n",drive,*(unsigned char *) (2+BIOS));
		//hd_info[drive].wpcom = *(unsigned short *) (5+BIOS);
		printk("drive = %d,hd_info[drive].wpcom = %d\n",drive,*(unsigned short *) (5+BIOS));
		//hd_info[drive].ctl = *(unsigned char *) (8+BIOS);
		printk("drive = %d,hd_info[drive].ctl = %d\n",drive,*(unsigned char *) (8+BIOS));
		//hd_info[drive].lzone = *(unsigned short *) (12+BIOS);
		printk("drive = %d,hd_info[drive].lzone = %d\n",drive,*(unsigned short *) (12+BIOS));
		//hd_info[drive].sect = *(unsigned char *) (14+BIOS);
		printk("drive = %d,hd_info[drive].sect = %d\n",drive,*(unsigned char *) (14+BIOS));
		BIOS += 16;
	}
}
