#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/tty.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <linux/console_struct.h>

#define DRIVER_AUTHOR "Romain Porte <microjoe@mailoo.org>"
#define DRIVER_DESC   "A sample driver making the keyboard blink"

/* Integer containing the active TTY number */
extern int fg_console;

struct timer_list my_timer;
struct tty_driver *my_driver;
char kbledstatus = 0;

/* The state will change with a 1/5 (0.2) Hz frequency. Do not go over 1/15 as
 * the keyboard driver will not be able to follow the required switch
 * frequency. */

#define BLINK_DELAY  HZ/5

/* The LED state is stored on the 3 first bits of a byte, in the same order
 * that on the keyboard LEDs are disposed:
 *
 * Bit 2 : CapsLock LED
 * Bit 1 : NumLock LED
 * Bit 0 : ScrollLock LED
 *
 * Example: set all LEDs ON: 0x07 -> 0b0000 0111
 *
 * Any value over 0x07 will reset the LED in order to indicate the state of the
 * keyboard.
 */

#define ALL_LEDS_ON  0x04
#define ALL_LEDS_OFF 0x02
#define RESTORE_LEDS 0xFF /* Restore all the LEDs to their status state */

static void my_timer_func(unsigned long ptr)
{
	int *pstatus = (int *) ptr;

	/* Switch the LED state */
	if (*pstatus == ALL_LEDS_ON)
		*pstatus = ALL_LEDS_OFF;
	else
		*pstatus = ALL_LEDS_ON;

	/* Call ioctl on the driver in order to update the LED state */
	((my_driver->ops)->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, *pstatus);

	/* Update the timer in order to call this function again */
	my_timer.expires = jiffies + BLINK_DELAY;
	add_timer(&my_timer);
}

int init_module(void)
{
	int i;

	printk(KERN_DEBUG "kbleds: fgconsole is %x\n", fg_console);

	for (i = 0; i < MAX_NR_CONSOLES; i++) {
		if (!vc_cons[i].d)
			break;
	}

	/* Retrieve the driver for the active console */
	my_driver = vc_cons[fg_console].d->port.tty->driver;

	/* Set up timer callback for blinking */
	init_timer(&my_timer);
	my_timer.function = my_timer_func;
	my_timer.data = (unsigned long) &kbledstatus;
	my_timer.expires = jiffies + BLINK_DELAY;
	add_timer(&my_timer);

	return 0;
}

void cleanup_module(void)
{
	/* Remove callback timer */
	del_timer(&my_timer);

	/* Restore LED state */
	/* Possible bug: the LEDs does not light up back and need a keyboard change
	 * before resetting state */
	((my_driver->ops)->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, RESTORE_LEDS);
}

MODULE_LICENSE("GPL");

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
