/**
 * @file felix/posix/grp.c
 * @brief POSIX group management shims for Felix OS.
 */

#include <grp.h>
#include <stddef.h>

/**
 * @brief Fills the array with the group IDs the user belongs to.
 * * In Felix OS (a single root-user system), there are no secondary
 * groups. The user simply belongs to the primary group provided.
 * * @param user   The username (ignored in Felix OS)
 * @param group  The primary GID of the user
 * @param groups The array to fill with GIDs
 * @param ngroups Pointer to the size of the array, updated to the number of groups
 * @return The number of groups, or -1 if the array was too small.
 */
int getgrouplist(const char *user, gid_t group, gid_t *groups, int *ngroups) {
    if (!groups || !ngroups) return -1;

    /* The user only ever has 1 group */
    int num_groups = 1;

    /* If the provided array is too small, update ngroups and return -1 */
    if (*ngroups < num_groups) {
        *ngroups = num_groups;
        return -1;
    }

    /* Assign the primary group and update the count */
    groups[0] = group;
    *ngroups = num_groups;

    return num_groups;
}
