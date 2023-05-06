#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(int argc, const char** argv)
{
    const char* progname = argc > 0 ? argv[0] : "embed";
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s [options] <input file|->\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -n <string>   -- constant name (default: \"data\")\n");
    fprintf(stderr, "  -i <string>   -- indentation (default: \"    \" [4 spaces])\n");
    fprintf(stderr, "  -w <number>   -- maximum line width (default: 80)\n");
    fprintf(stderr, "  -c <filename> -- source output file (append, default: stdout)\n");
    fprintf(stderr, "  -h <filename> -- header output file (append)\n");
    fprintf(stderr, "  -t <filename> -- title in comment (default: <input file>)\n");
    fprintf(stderr, "Notes:\n");
    fprintf(stderr, "  - you can use '-' as an input file for stdin\n");
    fprintf(stderr, "  - non-alphanumeric characters in the constant name will automatically be replaced by underscores\n");
    fprintf(stderr, "  - declaration automatically becomes static if no header output file is given\n");
}

int main(int argc, const char** argv)
{
    int exit_code = 0;
    unsigned char* buf = NULL;
    FILE* c_output = stdout;
    FILE* h_output = NULL;

    // Parse options
    const char* input_filename = NULL;
    const char* c_output_filename = NULL;
    const char* h_output_filename = NULL;
    const char* indent = "    ";
    const char* name = "data";
    const char* title = NULL;
    int max_width = 80;
    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        const char* param = i + 1 < argc ? argv[i + 1] : NULL;
        if (arg[0] == 0)
            continue;
        if (arg[0] == '-' && arg[1]) {
            for (int j = 1; arg[j]; j++) {
                bool chars_following = arg[j + 1] != 0;
                if (chars_following)
                    param = &arg[j + 1];
                bool param_used = false;
                bool param_missing = false;
                switch (arg[j]) {
                case 'n':
                    if (param) {
                        name = param;
                        param_used = true;
                    } else
                        param_missing = true;
                    break;
                case 'i':
                    if (param) {
                        indent = param;
                        param_used = true;
                    } else
                        param_missing = true;
                    break;
                case 'w':
                    if (param) {
                        max_width = atoi(param);
                        param_used = true;
                    } else {
                        param_missing = true;
                    }
                    break;
                case 'c':
                    if (param) {
                        c_output_filename = param;
                        param_used = true;
                    } else
                        param_missing = true;
                    break;
                case 'h':
                    if (param) {
                        h_output_filename = param;
                        param_used = true;
                    } else
                        param_missing = true;
                    break;
                case 't':
                    if (param) {
                        title = param;
                        param_used = true;
                    } else
                        param_missing = true;
                    break;
                default:
                    fprintf(stderr, "Invalid option: '-%c'\n", arg[j]);
                    print_usage(argc, argv);
                    goto deinit_error;
                }
                if (param_missing) {
                    fprintf(stderr, "Expected parameter after '-%c'\n", arg[j]);
                    print_usage(argc, argv);
                    goto deinit_error;
                }
                if (param_used) {
                    if (chars_following)
                        break;
                    else
                        i++;
                }
            }
        } else {
            if (input_filename) {
                print_usage(argc, argv);
                goto deinit_error;
            }
            input_filename = arg;
        }
    }
    if (!input_filename) {
        fprintf(stderr, "Expected input file\n");
        print_usage(argc, argv);
        goto deinit_error;
    }

    if (!title)
        title = input_filename;

    // Make constant name valid
    char name_buf[256];
    for (size_t i = 0;; i++) {
        if (i >= 256) {
            fprintf(stderr, "Constant name too long (%d/256)\n", (int)strlen(name));
            goto deinit_error;
        }
        if (!name[i]) {
            name_buf[i] = 0;
            break;
        }
        name_buf[i] = (isdigit(name[i]) && i > 0) || isalpha(name[i]) ? name[i] : '_';
    }
    name = name_buf;

    FILE* input;
    if (strcmp(input_filename, "-") == 0)
        input = stdin;
    else {
        input = fopen(input_filename, "rb");
        if (!input) {
            fprintf(stderr, "Error opening %s: %s\n", input_filename, strerror(errno));
            goto deinit_error;
        }
    }

    // Read input (not using seek because stdin doesn't support seek)
    long len = 0, cap = 2048;
    buf = malloc(cap);
    for (int c; (c = getc(input)) != EOF;) {
        if (len + 1 > cap)
            buf = realloc(buf, cap *= 2);
        buf[len++] = c;
    }

    fclose(input);

    if (c_output_filename) {
        c_output = fopen(c_output_filename, "a");
        if (!c_output) {
            fprintf(stderr, "Error opening %s: %s\n", c_output_filename, strerror(errno));
            goto deinit_error;
        }
    }

    if (h_output_filename) {
        h_output = fopen(h_output_filename, "a");
        if (!h_output) {
            fprintf(stderr, "Error opening %s: %s\n", h_output_filename, strerror(errno));
            goto deinit_error;
        }
    }

    // Write C header output
    if (h_output) {
        fprintf(h_output, "/* %s */\n", title);
        fprintf(h_output, "extern const unsigned char %s[%ld];\n\n", name, len);
    }

    // Write C source output
    int width = 0;
    int byte_chars = strlen("0x00, ");
    int indent_chars = strlen(indent);
    if (indent_chars + byte_chars > max_width)
        max_width = indent_chars + byte_chars;
    fprintf(c_output, "/* %s */\n", title);
    if (!h_output)
        fprintf(c_output, "static ");
    fprintf(c_output, "const unsigned char %s[%ld] = {", name, len);
    for (size_t i = 0; i < (size_t)len; i++) {
        if (i > 0)
            fprintf(c_output, ", ");
        if (i == 0 || width + byte_chars > max_width) {
            width = indent_chars;
            fprintf(c_output, "\n%s", indent);
        }
        fprintf(c_output, "0x%02x", buf[i]);
        width += byte_chars;
    }
    fprintf(c_output, "\n};\n\n");

deinit:
    if (h_output)
        fclose(h_output);
    fclose(c_output);
    free(buf);
    return exit_code;
deinit_error:
    exit_code = 1;
    goto deinit;
}
