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

int tcflush(int fd, int queue_selector) {
    (void)fd;

    /* Validate the queue_selector parameter as required by POSIX */
    if (queue_selector != TCIFLUSH &&
        queue_selector != TCOFLUSH &&
        queue_selector != TCIOFLUSH) {
        errno = EINVAL;
        return -1;
    }

    /* Felix OS serial drivers currently handle their own buffering,
     * so we safely pretend the flush succeeded. */
    return 0;
}

/* Optional: cfgetospeed/cfsetospeed usually macros or simple accessors */
speed_t cfgetispeed(const struct termios *termios_p) { return termios_p->__c_ispeed; }
speed_t cfgetospeed(const struct termios *termios_p) { return termios_p->__c_ospeed; }
int cfsetispeed(struct termios *termios_p, speed_t speed) { termios_p->__c_ispeed = speed; return 0; }
int cfsetospeed(struct termios *termios_p, speed_t speed) { termios_p->__c_ospeed = speed; return 0; }

int cfsetspeed(struct termios *termios_p, speed_t speed) {
    termios_p->__c_ispeed = speed;
    termios_p->__c_ospeed = speed;
    return 0;
}

void cfmakeraw(struct termios *termios_p) {
    /* Clear flags to put the terminal into raw, unbuffered mode */
    termios_p->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    termios_p->c_oflag &= ~OPOST;
    termios_p->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    termios_p->c_cflag &= ~(CSIZE | PARENB);
    termios_p->c_cflag |= CS8;

    /* Request immediate delivery of characters */
    termios_p->c_cc[VMIN] = 1;
    termios_p->c_cc[VTIME] = 0;
}
