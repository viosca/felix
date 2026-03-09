#include <sys/stat.h>
#include <string.h>
#include <stddef.h>
#include "kernel/vfs.h"
#include "kernel/process.h"

/* =========================================================================
 * Path Resolution & Traversal Subsystem (Name-to-Inode)
 * ========================================================================= */

char *flx_vfs_get_path(FlxVfsNode_t *node, char *buf, size_t size) {
    if (!node || !buf || size == 0) return NULL;

    if (node->parent == node) {
        strncpy(buf, "/", size);
        return buf;
    }

    /* Build string backwards from the end of the buffer */
    char *ptr = buf + size - 1;
    *ptr = '\0';

    FlxVfsNode_t *curr = node;
    while (curr->parent != curr) {
        size_t len = strlen(curr->name);
        if (ptr - buf < (ptrdiff_t)(len + 1)) return NULL; /* Buffer too small */

        ptr -= len;
        memcpy(ptr, curr->name, len);
        ptr--;
        *ptr = '/';

        curr = curr->parent;
    }

    /* Shift the completed string to the front of the buffer */
    memmove(buf, ptr, (buf + size) - ptr);
    return buf;
}

/* WWLD: The True POSIX Component-by-Component Path Walker */
FlxVfsNode_t *flx_vfs_path_resolve(FlxVfsNode_t *start_node, const char *path) {
    if (!path || *path == '\0') return NULL;

    FlxVfsNode_t *current = start_node;

    /* If it's an absolute path, jump to the root node immediately. */
    if (*path == '/') {
        current = flx_vfs_root_get();
    }

    /* Parse the path component by component (in-place, no allocations!) */
    while (*path != '\0') {
        /* Find the end of the current component */
        const char *end = path;
        while (*end != '/' && *end != '\0') {
            end++;
        }

        size_t len = end - path;

        /* Process the isolated component */
        if (len == 0 || (len == 1 && path[0] == '.')) {
            /* Ignore multiple slashes "//", leading slashes "/", or current dir "." */
        } else if (len == 2 && path[0] == '.' && path[1] == '.') {
            /* Go up one level "..". Because we circular-linked root->parent to root,
             * this safely stops at the root directory! */
            current = current->parent;
        } else {
            /* Normal directory traversal */
            if (!S_ISDIR(current->mode)) return NULL; /* Can't walk into a file! */

            FlxVfsNode_t **children = current->dir.children;
            FlxVfsNode_t *found = NULL;

            if (children) {
                for (int i = 0; children[i] != NULL; i++) {
                    if (strlen(children[i]->name) == len &&
                        strncmp(children[i]->name, path, len) == 0) {
                        found = children[i];
                        break;
                    }
                }
            }

            if (!found) return NULL; /* Component doesn't exist in this directory */
            current = found;
        }

        /* Advance the path pointer to the next component */
        path = end;
        if (*path == '/') path++; /* Step over the delimiter */
    }

    return current;
}

int flx_vfs_chdir(const char *path) {
    /* Resolve the target path relative to the current working directory */
    FlxVfsNode_t *target_node = flx_vfs_path_resolve(flx_proc_cwd_node_get(), path);

    if (!target_node) {
        return -1; /* Path doesn't exist */
    }

    /* Ensure the target is actually a directory before we CD into it */
    if (!S_ISDIR(target_node->mode)) {
        return -1;
    }

    /* Update the Process Control Block with the new CWD node */
    flx_proc_cwd_node_set(target_node);

    return 0;
}

char *flx_vfs_getcwd(char *buf, size_t size) {
    if (size == 0) return NULL;

    /* If buf is NULL, POSIX says we should malloc it.
     * Since we are avoiding kernel mallocs, we enforce providing a buffer. */
    if (!buf) {
        return NULL;
    }

    /* Trace the current process's CWD node back to root */
    return flx_vfs_get_path(flx_proc_cwd_node_get(), buf, size);
}
