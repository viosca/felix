/**
 * @file termios.c
 * @brief POSIX Terminal Control Implementation.
 * Maps tcgetattr/tcsetattr to ioctl calls for Felix OS.
 */

#include <sys/types.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>

/* * These functions are usually provided by libc, but in a bare-metal
 * newlib environment, they might be missing or stubbed.
 * We implement them here to satisfy linenoise.c linkage.
 */

int tcgetattr(int fd, struct termios *termios_p) {
    return ioctl(fd, TCGETS, termios_p);
}

int tcsetattr(int fd, int optional_actions, const struct termios *termios_p) {
    (void)optional_actions; /* We ignore flush/drain variants for now */
    return ioctl(fd, TCSETS, (void *)termios_p);
}

/* Optional: cfgetospeed/cfsetospeed usually macros or simple accessors */
speed_t cfgetispeed(const struct termios *termios_p) { return termios_p->__c_ispeed; }
speed_t cfgetospeed(const struct termios *termios_p) { return termios_p->__c_ospeed; }
int cfsetispeed(struct termios *termios_p, speed_t speed) { termios_p->__c_ispeed = speed; return 0; }
int cfsetospeed(struct termios *termios_p, speed_t speed) { termios_p->__c_ospeed = speed; return 0; }
