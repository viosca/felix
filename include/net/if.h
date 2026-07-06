/**
 * @file felix/include/net/if.h
 * @brief POSIX standard network interface operations.
 */
#ifndef _NET_IF_H
#define _NET_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/* POSIX mandated constant for maximum interface name length */
#define IF_NAMESIZE 16

/* POSIX structure for network interface index and name */
struct if_nameindex {
    unsigned int if_index; /* 1, 2, ... */
    char        *if_name;  /* null-terminated name: "eth0", etc. */
};

/* POSIX Function Prototypes (Stubs for future networking stack) */
unsigned int  if_nametoindex(const char *ifname);
char         *if_indextoname(unsigned int ifindex, char *ifname);
struct if_nameindex *if_nameindex(void);
void          if_freenameindex(struct if_nameindex *ptr);

#ifdef __cplusplus
}
#endif

#endif /* _NET_IF_H */
