/**
 * @file wordexp.c
 * @brief Reentrant state-machine implementation of wordexp.
 */

#include "wordexp.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Parser states */
typedef enum {
    STATE_NORMAL,
    STATE_IN_SQUOTE,
    STATE_IN_DQUOTE
} parser_state_t;

/* Dynamic buffer for assembling the current word (Malloc Mode) */
typedef struct {
    char *data;
    size_t len;
    size_t cap;
} word_buffer_t;

static int buf_append(word_buffer_t *buf, char c) {
    if (buf->len + 1 >= buf->cap) {
        size_t new_cap = buf->cap == 0 ? 32 : buf->cap * 2;
        char *new_data = realloc(buf->data, new_cap);
        if (!new_data) return -1;
        buf->data = new_data;
        buf->cap = new_cap;
    }
    buf->data[buf->len++] = c;
    buf->data[buf->len] = '\0';
    return 0;
}

static void buf_free(word_buffer_t *buf) {
    if (buf->data) free(buf->data);
    buf->data = NULL;
    buf->len = 0;
    buf->cap = 0;
}

/* Helper to free a partially or fully constructed wordexp_t */
void wordfree(wordexp_t *p) {
    if (!p || !p->we_wordv) return;

    /* Free the strings we allocated */
    for (size_t i = p->we_offs; i < p->we_offs + p->we_wordc; i++) {
        if (p->we_wordv[i]) free(p->we_wordv[i]);
    }

    free(p->we_wordv);
    p->we_wordv = NULL;
    p->we_wordc = 0;
}

/* Add a completed word to the wordexp_t array (Malloc Mode) */
static int add_word(wordexp_t *p, word_buffer_t *buf) {
    size_t total_words = p->we_offs + p->we_wordc;

    char **new_wordv = realloc(p->we_wordv, (total_words + 2) * sizeof(char *));
    if (!new_wordv) return -1;
    p->we_wordv = new_wordv;

    p->we_wordv[total_words] = strdup(buf->data ? buf->data : "");
    if (!p->we_wordv[total_words]) return -1;

    p->we_wordc++;
    p->we_wordv[total_words + 1] = NULL;

    buf->len = 0;
    if (buf->data) buf->data[0] = '\0';
    return 0;
}

/* Check if character is a shell special character */
static int is_bad_char(char c) {
    return (c == '\n' || c == '|' || c == '&' || c == ';' ||
            c == '<'  || c == '>' || c == '(' || c == ')' ||
            c == '{'  || c == '}');
}

/* --- Shared Parsing Engine Context --- */
typedef struct {
    int mode; /* 0=malloc, 1=measure, 2=fill */
    wordexp_t *p;
    word_buffer_t dyn_buf;

    size_t req_words;
    size_t req_bytes;

    char **argv_buf;
    size_t argv_cap;
    char *str_buf;
    size_t str_cap;

    size_t word_idx;
    size_t byte_idx;
    size_t word_start_byte;
} we_ctx_t;

static size_t ctx_word_len(we_ctx_t *ctx) {
    if (ctx->mode == 0) return ctx->dyn_buf.len;
    if (ctx->mode == 1) return ctx->req_bytes - ctx->word_start_byte;
    return ctx->byte_idx - ctx->word_start_byte;
}

static int ctx_append_char(we_ctx_t *ctx, char c) {
    if (ctx->mode == 0) {
        return buf_append(&ctx->dyn_buf, c) == 0 ? WRDE_SUCCESS : WRDE_NOSPACE;
    } else if (ctx->mode == 1) {
        ctx->req_bytes++;
        return WRDE_SUCCESS;
    } else {
        if (ctx->byte_idx >= ctx->str_cap) return WRDE_NOSPACE;
        ctx->str_buf[ctx->byte_idx++] = c;
        return WRDE_SUCCESS;
    }
}

static int ctx_append_str(we_ctx_t *ctx, const char *str) {
    while (*str) {
        int err = ctx_append_char(ctx, *str++);
        if (err != WRDE_SUCCESS) return err;
    }
    return WRDE_SUCCESS;
}

static int ctx_end_word(we_ctx_t *ctx) {
    if (ctx->mode == 0) {
        return add_word(ctx->p, &ctx->dyn_buf) == 0 ? WRDE_SUCCESS : WRDE_NOSPACE;
    } else if (ctx->mode == 1) {
        ctx->req_words++;
        ctx->req_bytes++; /* Space for null terminator */
        ctx->word_start_byte = ctx->req_bytes;
        return WRDE_SUCCESS;
    } else {
        if (ctx->byte_idx >= ctx->str_cap) return WRDE_NOSPACE;
        ctx->str_buf[ctx->byte_idx++] = '\0';

        if (ctx->word_idx >= ctx->argv_cap) return WRDE_NOSPACE;
        ctx->argv_buf[ctx->word_idx++] = &ctx->str_buf[ctx->word_start_byte];

        ctx->word_start_byte = ctx->byte_idx;
        return WRDE_SUCCESS;
    }
}

/* Core unified parser engine */
static int parse_engine(const char *s, int flags, we_ctx_t *ctx) {
    parser_state_t state = STATE_NORMAL;
    const char *ptr = s;
    int err = WRDE_SUCCESS;

    while (*ptr != '\0') {
        char c = *ptr++;

        switch (state) {
            case STATE_NORMAL:
                if (isspace((unsigned char)c)) {
                    if (ctx_word_len(ctx) > 0) {
                        if ((err = ctx_end_word(ctx)) != WRDE_SUCCESS) goto cleanup;
                    }
                } else if (c == '\\') {
                    if (*ptr == '\0') { err = WRDE_SYNTAX; goto cleanup; }
                    if ((err = ctx_append_char(ctx, *ptr++)) != WRDE_SUCCESS) goto cleanup;
                } else if (c == '\'') {
                    state = STATE_IN_SQUOTE;
                } else if (c == '"') {
                    state = STATE_IN_DQUOTE;
                } else if (c == '$') {
                    if (*ptr == '(') { err = WRDE_CMDSUB; goto cleanup; }
                    char var_name[64];
                    size_t vlen = 0;
                    while (isalnum((unsigned char)*ptr) || *ptr == '_') {
                        if (vlen < sizeof(var_name) - 1) var_name[vlen++] = *ptr;
                        ptr++;
                    }
                    var_name[vlen] = '\0';
                    char *val = getenv(var_name);
                    if (!val && (flags & WRDE_UNDEF)) { err = WRDE_BADVAL; goto cleanup; }
                    if (val && (err = ctx_append_str(ctx, val)) != WRDE_SUCCESS) goto cleanup;
                } else if (c == '`') {
                    err = WRDE_CMDSUB; goto cleanup;
                } else if (is_bad_char(c)) {
                    err = WRDE_BADCHAR; goto cleanup;
                } else {
                    if ((err = ctx_append_char(ctx, c)) != WRDE_SUCCESS) goto cleanup;
                }
                break;

            case STATE_IN_SQUOTE:
                if (c == '\'') {
                    state = STATE_NORMAL;
                } else {
                    if ((err = ctx_append_char(ctx, c)) != WRDE_SUCCESS) goto cleanup;
                }
                break;

            case STATE_IN_DQUOTE:
                if (c == '"') {
                    state = STATE_NORMAL;
                } else if (c == '\\') {
                    if (*ptr == '$' || *ptr == '`' || *ptr == '"' || *ptr == '\\') {
                        if ((err = ctx_append_char(ctx, *ptr++)) != WRDE_SUCCESS) goto cleanup;
                    } else {
                        if ((err = ctx_append_char(ctx, '\\')) != WRDE_SUCCESS) goto cleanup;
                    }
                } else if (c == '$') {
                    char var_name[64];
                    size_t vlen = 0;
                    while (isalnum((unsigned char)*ptr) || *ptr == '_') {
                        if (vlen < sizeof(var_name) - 1) var_name[vlen++] = *ptr;
                        ptr++;
                    }
                    var_name[vlen] = '\0';
                    char *val = getenv(var_name);
                    if (!val && (flags & WRDE_UNDEF)) { err = WRDE_BADVAL; goto cleanup; }
                    if (val && (err = ctx_append_str(ctx, val)) != WRDE_SUCCESS) goto cleanup;
                } else if (c == '`') {
                    err = WRDE_CMDSUB; goto cleanup;
                } else {
                    if ((err = ctx_append_char(ctx, c)) != WRDE_SUCCESS) goto cleanup;
                }
                break;
        }
    }

    if (state != STATE_NORMAL) {
        err = WRDE_SYNTAX;
        goto cleanup;
    }

    if (ctx_word_len(ctx) > 0) {
        if ((err = ctx_end_word(ctx)) != WRDE_SUCCESS) goto cleanup;
    }

cleanup:
    return err;
}

/* Standard Malloc-based Entry Point */
int wordexp(const char *s, wordexp_t *p, int flags) {
    if (!s || !p) return WRDE_SYNTAX;

    if ((flags & WRDE_REUSE) && !(flags & WRDE_APPEND)) {
        wordfree(p);
    }

    if (!(flags & WRDE_APPEND)) {
        p->we_wordc = 0;
        p->we_offs = (flags & WRDE_DOOFFS) ? p->we_offs : 0;
        p->we_wordv = calloc(p->we_offs + 1, sizeof(char *));
        if (!p->we_wordv) return WRDE_NOSPACE;
    }

    we_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.mode = 0;
    ctx.p = p;

    int err = parse_engine(s, flags, &ctx);

    buf_free(&ctx.dyn_buf);
    if (err != WRDE_SUCCESS && !(flags & WRDE_APPEND)) {
        wordfree(p);
    }
    return err;
}

/* Stack/VLA Zero-Allocation Entry Point */
int wordexp_noalloc(const char *s, wordexp_t *p, int flags,
                    char **argv_buf, size_t argv_cap,
                    char *str_buf, size_t str_cap,
                    size_t *req_words, size_t *req_bytes) {
    if (!s) return WRDE_SYNTAX;

    we_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    if (!argv_buf && !str_buf) {
        /* Pass 1: Measure */
        ctx.mode = 1;
        if ((flags & WRDE_DOOFFS) && p) ctx.req_words = p->we_offs;

        int err = parse_engine(s, flags, &ctx);

        if (err == WRDE_SUCCESS) {
            if (req_words) *req_words = ctx.req_words + 1; /* +1 for NULL terminator */
            if (req_bytes) *req_bytes = ctx.req_bytes;
        }
        return err;
    } else {
        /* Pass 2: Fill */
        if (!p) return WRDE_SYNTAX;

        ctx.mode = 2;
        ctx.argv_buf = argv_buf;
        ctx.argv_cap = argv_cap;
        ctx.str_buf = str_buf;
        ctx.str_cap = str_cap;

        if (flags & WRDE_APPEND) {
            ctx.word_idx = (flags & WRDE_DOOFFS ? p->we_offs : 0) + p->we_wordc;
        } else if (flags & WRDE_DOOFFS) {
            ctx.word_idx = p->we_offs;
            for (size_t i = 0; i < p->we_offs && i < argv_cap; i++) argv_buf[i] = NULL;
        }

        int err = parse_engine(s, flags, &ctx);

        if (err == WRDE_SUCCESS) {
            if (ctx.word_idx < ctx.argv_cap) {
                ctx.argv_buf[ctx.word_idx] = NULL;
            } else {
                return WRDE_NOSPACE;
            }

            if (!(flags & WRDE_APPEND)) {
                p->we_wordc = ctx.word_idx - ((flags & WRDE_DOOFFS) ? p->we_offs : 0);
                p->we_wordv = ctx.argv_buf;
            } else {
                p->we_wordc += (ctx.word_idx - ((flags & WRDE_DOOFFS ? p->we_offs : 0) + p->we_wordc));
            }
        }
        return err;
    }
}
