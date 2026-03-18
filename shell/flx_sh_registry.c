#include <string.h>
#include <stddef.h>
#include "shell/flx_sh_registry.h"

#define FLX_MAX_SH_APPS 64

typedef struct {
    const char *name;
    flx_sh_complete_cb completer;
    const char *help_text;
} FlxCmdMeta_t;

static FlxCmdMeta_t g_registry[FLX_MAX_SH_APPS];
static int g_registry_count = 0;

int flx_sh_registry_add(const char *name, flx_sh_complete_cb completer, const char *help_text) {
    if (g_registry_count >= FLX_MAX_SH_APPS || !name) return -1;

    int i = g_registry_count++;
    g_registry[i].name = name;
    g_registry[i].completer = completer;
    g_registry[i].help_text = help_text;

    return 0;
}

flx_sh_complete_cb flx_sh_registry_get_completer(const char *name) {
    if (!name) return NULL;

    for (int i = 0; i < g_registry_count; i++) {
        if (strcmp(g_registry[i].name, name) == 0) {
            return g_registry[i].completer;
        }
    }
    return NULL;
}

const char* flx_sh_registry_get_help(const char *name) {
    if (!name) return NULL;

    for (int i = 0; i < g_registry_count; i++) {
        if (strcmp(g_registry[i].name, name) == 0) {
            return g_registry[i].help_text;
        }
    }
    return NULL;
}
