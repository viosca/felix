#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include "shell/flx_sh_history.h"
#include "linenoise.h"

bool flx_sh_history_expand(const char *input, char *output, size_t out_len) {
    if (!input || !output || out_len == 0) return false;

    output[0] = '\0';
    size_t out_pos = 0;
    bool expanded = false;

    int hist_count = linenoiseHistoryGetCount();

    for (size_t i = 0; input[i] != '\0'; i++) {
        /* Handle escaped exclamation marks (e.g., echo "Hello \!") */
        if (input[i] == '\\' && input[i+1] == '!') {
            if (out_pos < out_len - 1) output[out_pos++] = '!';
            i++; /* Skip the '!' so the loop doesn't re-process it */
            continue;
        }

        /* Detect history expansion token '!' */
        if (input[i] == '!' && input[i+1] != '\0' && input[i+1] != ' ') {
            const char *hist_cmd = NULL;
            char last_arg[256] = {0};
            size_t skip = 1; /* How many characters to jump after '!' */

            if (hist_count == 0) {
                printf("bash: %s: event not found\n", &input[i]);
                return false;
            }

            /* !! repeats the last command */
            if (input[i+1] == '!') {
                hist_cmd = linenoiseHistoryGet(hist_count - 1);
                skip = 1;
            }
            /* !$ gets the last argument of the last command */
            else if (input[i+1] == '$') {
                const char *last = linenoiseHistoryGet(hist_count - 1);
                if (last && *last) {
                    /* Strip trailing spaces to find the true end of the line */
                    int end = strlen(last) - 1;
                    while (end >= 0 && last[end] == ' ') end--;

                    /* Trace backward to find the space just before the last word */
                    int start = end;
                    while (start >= 0 && last[start] != ' ') start--;

                    /* Isolate and copy just the last argument! */
                    if (end >= 0 && start < end) {
                        int len = end - start;
                        if (len >= (int)sizeof(last_arg)) len = sizeof(last_arg) - 1;
                        strncpy(last_arg, &last[start + 1], len);
                        last_arg[len] = '\0';
                        hist_cmd = last_arg;
                    }
                }
                skip = 1;
            }
            /* !-n repeats nth previous command */
            else if (input[i+1] == '-' && isdigit((unsigned char)input[i+2])) {
                int offset = atoi(&input[i+2]);
                if (offset > 0 && offset <= hist_count) {
                    hist_cmd = linenoiseHistoryGet(hist_count - offset);
                }
                while (isdigit((unsigned char)input[i + 1 + skip])) skip++;
            }
            /* !n repeats command number 'n' */
            else if (isdigit((unsigned char)input[i+1])) {
                int ev = atoi(&input[i+1]);
                if (ev > 0 && ev <= hist_count) {
                    hist_cmd = linenoiseHistoryGet(ev - 1);
                }
                while (isdigit((unsigned char)input[i + 1 + skip])) skip++;
            }
            /* !string repeats most recent command starting with string */
            else {
                size_t pre_len = 0;
                while (input[i + 1 + pre_len] != '\0' && input[i + 1 + pre_len] != ' ') pre_len++;

                for (int h = hist_count - 1; h >= 0; h--) {
                    const char *cmd = linenoiseHistoryGet(h);
                    if (cmd && strncmp(cmd, &input[i+1], pre_len) == 0) {
                        hist_cmd = cmd;
                        break;
                    }
                }
                skip = pre_len;
            }

            /* Apply the substitution into the string buffer if we found a match */
            if (hist_cmd) {
                size_t cmd_len = strlen(hist_cmd);
                if (out_pos + cmd_len < out_len - 1) {
                    strcpy(&output[out_pos], hist_cmd);
                    out_pos += cmd_len;
                    expanded = true;
                }
                i += skip; /* Jump the parser over the history token we just processed */
                continue;
            } else {
                /* To mimic bash closely, isolate the bad token and fail */
                char bad_token[64] = {0};
                size_t bad_len = skip + 1;
                if (bad_len >= sizeof(bad_token)) bad_len = sizeof(bad_token) - 1;

                strncpy(bad_token, &input[i], bad_len);
                printf("bash: %s: event not found\n", bad_token);
                return false;
            }
        }

        /* Normal character copy */
        if (out_pos < out_len - 1) {
            output[out_pos++] = input[i];
        }
    }

    output[out_pos] = '\0';

    /* If we performed an expansion, bash echoes the resulting parsed command */
    if (expanded) {
        printf("%s\n", output);
    }

    return true;
}
#if 0
bool flx_sh_history_expand(const char *input, char *output, size_t out_len) {
    if (!input || !output || out_len == 0) return false;

    /* If it doesn't start with '!', no substitution is needed. Just copy it over. */
    if (input[0] != '!') {
        strncpy(output, input, out_len - 1);
        output[out_len - 1] = '\0';
        return true;
    }

    int hist_count = linenoiseHistoryGetCount();
    const char *hist_cmd = NULL;

    /* !! repeats the last command */
    if (input[1] == '!') {
        hist_cmd = linenoiseHistoryGet(hist_count - 1);
    }
    /* !-n repeats nth previous command */
    else if (input[1] == '-' && isdigit((unsigned char)input[2])) {
        int offset = atoi(&input[2]);
        hist_cmd = linenoiseHistoryGet(hist_count - offset);
    }
    /* !n repeats command number 'n' */
    else if (isdigit((unsigned char)input[1])) {
        int ev = atoi(&input[1]);
        hist_cmd = linenoiseHistoryGet(ev - 1);
    }
    /* !string repeats most recent command starting with string */
    else {
        for (int i = hist_count - 1; i >= 0; i--) {
            const char *cmd = linenoiseHistoryGet(i);
            if (cmd && strncmp(cmd, &input[1], strlen(&input[1])) == 0) {
                hist_cmd = cmd;
                break;
            }
        }
    }

    /* If we found a match, populate the output buffer and echo it like bash does */
    if (hist_cmd) {
        strncpy(output, hist_cmd, out_len - 1);
        output[out_len - 1] = '\0';
        printf("%s\n", output);
        return true;
    }

    /* Target event was not found in history */
    printf("bash: %s: event not found\n", input);
    return false;
}
#endif
