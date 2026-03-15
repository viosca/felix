#ifndef FELIX_BINFMT_H
#define FELIX_BINFMT_H

#include <stdint.h>

#define FLX_MAGIC_0 0x7F
#define FLX_MAGIC_1 'F'
#define FLX_MAGIC_2 'L'
#define FLX_MAGIC_3 'X'

#define FLX_BINFMT_TYPE_LINKED   0x01
#define FLX_BINFMT_TYPE_EXTERNAL 0x02

/**
 * @brief Macro to statically declare a linked executable payload in memory.
 */
#define FLX_BINFMT_DECLARE_PAYLOAD(payload_var, exec_func, comp_func, help_text) \
    static const FlxLinkedExec_t payload_var = { \
        .magic = { FLX_MAGIC_0, FLX_MAGIC_1, FLX_MAGIC_2, FLX_MAGIC_3 }, \
        .type = FLX_BINFMT_TYPE_LINKED, \
        .main_func = (exec_func), \
        .complete_func = (comp_func), \
        .short_help = (help_text) \
    }

/* Format for files that point to statically linked OS functions & metadata */
typedef struct {
    uint8_t magic[4];       /* \x7f F L X */
    uint8_t type;           /* 0x01 for "Linked Pointer" */

    /* The linker resolves all of these absolute addresses at compile time! */
    int  (*main_func)(int argc, char **argv);
    void (*complete_func)(const char *prefix, void *lc); /* For Tab Completion */
    const char *short_help;                              /* For the 'help' command */
} __attribute__((packed)) FlxLinkedExec_t;

/* Format for standalone apps loaded from external storage */
typedef struct {
    uint8_t  magic[4];       /* \x7f F L X */
    uint8_t  type;           /* 0x02 for "External Offset" */
    uint32_t entry_offset;   /* Byte offset from file start to main() */
    uint32_t help_offset;    /* Byte offset to the short_help string */
    uint32_t bss_size;       /* How much RAM to allocate for variables */
} __attribute__((packed)) FlxExternalExec_t;

/**
 * @brief Parses a file buffer and executes it if it is a valid FXF binary.
 * @return Exit status of the program, or -1 if the format is invalid.
 */
int flx_load_binary(void *file_data, int argc, char **argv);

#endif /* FELIX_BINFMT_H */
