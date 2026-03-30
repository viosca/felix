/**
 * @file binfmt.h
 * @brief Binary format and payload definitions.
 */

#ifndef FELIX_KERNEL_BINFMT_H
#define FELIX_KERNEL_BINFMT_H

#include <stdint.h>
#include <stddef.h>

#define FLX_MAGIC_0 0x7F
#define FLX_MAGIC_1 'F'
#define FLX_MAGIC_2 'L'
#define FLX_MAGIC_3 'X'

#define FLX_BINFMT_TYPE_LINKED 1
#define FLX_BINFMT_TYPE_EXTERNAL 2


typedef struct {
    uint8_t magic[4];
    uint32_t type;
    size_t size;           /* Fixed size (used for embedded data/text files) */
    const void *start_ptr; /* Section start (for dynamic command sizing) */
    const void *end_ptr;   /* Section end (for dynamic command sizing) */
    int (*main_func)(int argc, char **argv);
} FlxLinkedExec_t;

/* Format for standalone apps loaded from external storage */
typedef struct {
    uint8_t  magic[4];       /* \x7f F L X */
    uint32_t type;           /* 0x02 for "External Offset" */
    uint32_t entry_offset;   /* Byte offset from file start to main() */
    uint32_t bss_size;       /* How much RAM to allocate for variables */
} __attribute__((packed)) FlxExternalExec_t;

/**
 * @brief GCC Attribute to force a function into a specific command's text section.
 * By using a valid C identifier for the section name (no periods), the GNU Linker
 * will automatically generate __start_NAME and __stop_NAME symbols for us!
 */
#define FLX_BINFMT_CMD_SECTION(cmd_name) \
    __attribute__((section(#cmd_name "_sec")))

/**
 * @brief Declares the external linker-generated bounds for a command section.
 * Marked as weak so that if a developer omits the FLX_BINFMT_CMD_SECTION attribute,
 * the linker safely resolves these to NULL rather than throwing an undefined reference error.
 */
#define FLX_BINFMT_EXTERN_BOUNDS(cmd_name) \
    extern char __start_##cmd_name##_sec[] __attribute__((weak)); \
    extern char __stop_##cmd_name##_sec[] __attribute__((weak))

/**
 * @brief Runtime helper to calculate the byte size of a command section.
 * WARNING: This cannot be used in static struct initializers! Use it inside functions.
 */
#define FLX_BINFMT_CALC_SIZE(cmd_name) \
    ((size_t)__stop_##cmd_name##_sec - (size_t)__start_##cmd_name##_sec)

/**
 * @brief Macro to statically declare a LINKED EXECUTABLE payload in memory.
 * Stores the linker-generated section pointers so `stat()` can calculate the size dynamically.
 */
#define FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_var, exec_func, cmd_name) \
    static const FlxLinkedExec_t payload_var = { \
        .magic = { FLX_MAGIC_0, FLX_MAGIC_1, FLX_MAGIC_2, FLX_MAGIC_3 }, \
        .type = FLX_BINFMT_TYPE_LINKED, \
        .size = 0, \
        .start_ptr = __start_##cmd_name##_sec, \
        .end_ptr = __stop_##cmd_name##_sec, \
        .main_func = (exec_func) \
    }

/**
 * @brief Macro to statically declare a STANDARD payload in memory (like a text/config file).
 * Use this if you don't care about dynamic sizing or aren't using command sections.
 */
#define FLX_BINFMT_DECLARE_PAYLOAD(payload_var, exec_func, payload_size) \
    static const FlxLinkedExec_t payload_var = { \
        .magic = { FLX_MAGIC_0, FLX_MAGIC_1, FLX_MAGIC_2, FLX_MAGIC_3 }, \
        .type = FLX_BINFMT_TYPE_LINKED, \
        .size = (size_t)(payload_size), \
        .start_ptr = NULL, \
        .end_ptr = NULL, \
        .main_func = (exec_func) \
    }

int flx_binfmt_load(void *file_data, int argc, char **argv);

#endif /* FELIX_KERNEL_BINFMT_H */
