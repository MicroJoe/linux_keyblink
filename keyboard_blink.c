#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/tty.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <linux/console_struct.h>

#define DRIVER_AUTHOR "Romain Porte <microjoe@mailoo.org>"
#define DRIVER_DESC   "A sample driver making the keyboard blink"

extern int fg_console;

struct timer_list my_timer;
struct tty_driver *my_driver;
char kbledstatus = 0;

#define BLINK_DELAY  HZ/5
#define ALL_LEDS_ON  0x07
#define ALL_LEDS_OFF 0xFF
#define RESTORE_LEDS ALL_LEDS_OFF

static void my_timer_func(unsigned long ptr)
{
	int *pstatus = (int *) ptr;

	if (*pstatus == ALL_LEDS_ON)
		*pstatus = ALL_LEDS_OFF;
	else
		*pstatus = ALL_LEDS_ON;

	((my_driver->ops)->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, *pstatus);

	my_timer.expires = jiffies + BLINK_DELAY;
	add_timer(&my_timer);
}

int init_module(void)
{
	int i;

	printk(KERN_INFO "kbleds: fgconsole is %x\n", fg_console);
	for (i = 0; i < MAX_NR_CONSOLES; i++) {
		if (!vc_cons[i].d)
			break;
	}

	my_driver = vc_cons[fg_console].d->port.tty->driver;

	init_timer(&my_timer);
	my_timer.function = my_timer_func;
	my_timer.data = (unsigned long) &kbledstatus;
	my_timer.expires = jiffies + BLINK_DELAY;
	add_timer(&my_timer);

	return 0;
}

void cleanup_module(void)
{
	del_timer(&my_timer);
	((my_driver->ops)->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, RESTORE_LEDS);
}

MODULE_LICENSE("GPL");

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
