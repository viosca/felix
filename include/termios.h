
/**
 * @file termios.h
 * @brief POSIX-compliant Terminal I/O definitions for Felix OS.
 *
 * This file defines the structure and constants used to control asynchronous
 * communications ports. It mimics the standard POSIX <termios.h> header
 * to facilitate porting of standard Unix utilities (like ncurses, readline).
 */

#ifndef _FLX_INCLUDE_TERMIOS_H
#define _FLX_INCLUDE_TERMIOS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup termios_types Terminal Types
 * @brief Data types used in the terminal control structure.
 * @{
 */
typedef unsigned int  tcflag_t; /**< Type for terminal mode flags (32-bit) */
typedef unsigned char cc_t;     /**< Type for control characters (8-bit) */
typedef unsigned int  speed_t;  /**< Type for terminal baud rates */
/** @} */

/** @brief Size of the control character array (c_cc) */
#define NCCS 32

/**
 * @brief Main Terminal Control Structure.
 *
 * This structure contains all the configuration parameters for a terminal
 * device, including input/output processing, control modes, and special
 * control characters.
 */
struct termios {
    tcflag_t c_iflag;    /**< Input mode flags */
    tcflag_t c_oflag;    /**< Output mode flags */
    tcflag_t c_cflag;    /**< Control mode flags */
    tcflag_t c_lflag;    /**< Local mode flags */
    cc_t     c_line;     /**< Line discipline */
    cc_t     c_cc[NCCS]; /**< Control characters */
    speed_t  __c_ispeed; /**< Input baud rate (non-standard internal) */
    speed_t  __c_ospeed; /**< Output baud rate (non-standard internal) */
};

/**
 * @name Input Flags
 * @brief Flags for the c_iflag member. Controls basic input processing.
 * @{
 */
#define IGNBRK  0000001 /**< Ignore break condition */
#define BRKINT  0000002 /**< Signal interrupt on break */
#define IGNPAR  0000004 /**< Ignore characters with parity errors */
#define PARMRK  0000010 /**< Mark parity and framing errors */
#define INPCK   0000020 /**< Enable input parity check */
#define ISTRIP  0000040 /**< Strip 8th bit off characters */
#define INLCR   0000100 /**< Map NL to CR on input */
#define IGNCR   0000200 /**< Ignore CR */
#define ICRNL   0000400 /**< Map CR to NL on input */
#define IUCLC   0001000 /**< Map Upper to Lower on input (Legacy) */
#define IXON    0002000 /**< Enable start/stop output control */
#define IXANY   0004000 /**< Any character will restart after stop */
#define IXOFF   0010000 /**< Enable start/stop input control */
#define IMAXBEL 0020000 /**< Ring bell when input queue is full */
#define IUTF8   0040000 /**< Input is UTF8 */
/** @} */

/**
 * @name Output Flags
 * @brief Flags for the c_oflag member. Controls output processing.
 * @{
 */
#define OPOST   0000001 /**< Post-process output (enable output processing) */
#define OLCUC   0000002 /**< Map Lower to Upper on output */
#define ONLCR   0000004 /**< Map NL to CR-NL on output */
#define OCRNL   0000010 /**< Map CR to NL on output */
#define ONOCR   0000020 /**< No CR output at column 0 */
#define ONLRET  0000040 /**< NL performs CR function */
#define OFILL   0000100 /**< Use fill characters for delay */
#define OFDEL   0000200 /**< Fill is DEL, else NUL */

/** @} */

/**
 * @name Control Flags
 * @brief Flags for the c_cflag member. Controls hardware settings (baud, parity, etc).
 * @{
 */
#define CSIZE   0000060 /**< Character size mask */
#define   CS5   0000000 /**< 5 bits per byte */
#define   CS6   0000020 /**< 6 bits per byte */
#define   CS7   0000040 /**< 7 bits per byte */
#define   CS8   0000060 /**< 8 bits per byte */
#define CSTOPB  0000100 /**< Set two stop bits */
#define CREAD   0000200 /**< Enable receiver */
#define PARENB  0000400 /**< Parity enable */
#define PARODD  0001000 /**< Odd parity (if unset, even parity) */
#define HUPCL   0002000 /**< Hang up on last close */
#define CLOCAL  0004000 /**< Ignore modem status lines */
/** @} */

/**
 * @name Local Flags
 * @brief Flags for the c_lflag member. Controls terminal editing and signaling.
 * @{
 */
#define ISIG    0000001 /**< Enable signals (INTR, QUIT, SUSP) */
#define ICANON  0000002 /**< Canonical input (Line-based, Erasure, Kill processing) */
#define XCASE   0000004 /**< Canonical upper/lower appearance */
#define ECHO    0000010 /**< Enable echo */
#define ECHOE   0000020 /**< Echo erase character as error-correcting backspace */
#define ECHOK   0000040 /**< Echo KILL */
#define ECHONL  0000100 /**< Echo NL even if ECHO is off */
#define NOFLSH  0000200 /**< Disable flush after interrupt/quit */
#define TOSTOP  0000400 /**< Send SIGTTOU for background output */
#define ECHOCTL 0001000 /**< Echo control characters as ^char */
#define ECHOPRT 0002000 /**< Echo erased character as character erased */
#define ECHOKE  0004000 /**< KILL erases entire line */
#define FLUSHO  0010000 /**< Output being flushed */
#define PENDIN  0040000 /**< Retype pending input */
#define IEXTEN  0100000 /**< Enable extended input processing */
/** @} */

/**
 * @name Control Characters
 * @brief Indices into the c_cc array.
 * @{
 */
#define VINTR    0   /**< Interrupt character (usually Ctrl-C) */
#define VQUIT    1   /**< Quit character (usually Ctrl-\) */
#define VERASE   2   /**< Erase character (Backspace) */
#define VKILL    3   /**< Kill character (usually Ctrl-U) */
#define VEOF     4   /**< End-of-file character (usually Ctrl-D) */
#define VTIME    5   /**< Time to wait for data (tenths of seconds) */
#define VMIN     6   /**< Minimum number of characters to read */
#define VSWTC    7   /**< Switch character */
#define VSTART   8   /**< Start character (Ctrl-Q) */
#define VSTOP    9   /**< Stop character (Ctrl-S) */
#define VSUSP    10  /**< Suspend character (Ctrl-Z) */
#define VEOL     11  /**< End-of-line character */
#define VREPRINT 12  /**< Reprint character */
#define VDISCARD 13  /**< Discard character */
#define VWERASE  14  /**< Word erase character */
#define VLNEXT   15  /**< Literal next character */
#define VEOL2    16  /**< Alternate end-of-line character */
/** @} */

/**
 * @name tcsetattr Actions
 * @brief Constants for the optional_actions argument of tcsetattr().
 * @{
 */
#define TCSANOW   0 /**< Change attributes immediately */
#define TCSADRAIN 1 /**< Change after output is drained */
#define TCSAFLUSH 2 /**< Change after output is drained; flush pending input */
/** @} */

/**
 * @name tcflush Queue Selectors
 * @brief Constants for the queue_selector argument of tcflush().
 * @{
 */
#define TCIFLUSH  0 /**< Flush pending input */
#define TCOFLUSH  1 /**< Flush untransmitted output */
#define TCIOFLUSH 2 /**< Flush both */
/** @} */

/**
 * @name Baud Rate Constants
 * @brief Constants for cfsetospeed() and cfsetispeed().
 * @{
 */
#define B0      0       /**< Hang up */
#define B50     1       /**< 50 baud */
#define B75     2       /**< 75 baud */
#define B110    3       /**< 110 baud */
#define B134    4       /**< 134.5 baud */
#define B150    5       /**< 150 baud */
#define B200    6       /**< 200 baud */
#define B300    7       /**< 300 baud */
#define B600    8       /**< 600 baud */
#define B1200   9       /**< 1200 baud */
#define B1800   10      /**< 1800 baud */
#define B2400   11      /**< 2400 baud */
#define B4800   12      /**< 4800 baud */
#define B9600   13      /**< 9600 baud */
#define B19200  14      /**< 19200 baud */
#define B38400  15      /**< 38400 baud */
#define B57600  4097    /**< 57600 baud */
#define B115200 4098    /**< 115200 baud */
/** @} */

int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int optional_actions, const struct termios *termios_p);
int tcflush(int fd, int queue_selector);

speed_t cfgetispeed(const struct termios *termios_p);
speed_t cfgetospeed(const struct termios *termios_p);
int cfsetispeed(struct termios *termios_p, speed_t speed);
int cfsetospeed(struct termios *termios_p, speed_t speed);
int cfsetspeed(struct termios *termios_p, speed_t speed);

void cfmakeraw(struct termios *termios_p);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_TERMIOS_H */
