#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <md2h.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("md2h: try md2h --help for more information\n");
        return 1;
    }

    if (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) {
        cmd_help();
        return 0;
    }

    if (!strcmp(argv[1], "--version") || !strcmp(argv[1], "-V")) {
        cmd_version();
        return 0;
    }

    if (!strcmp(argv[1], "--verbose") || !strcmp(argv[1], "-v")) {
        printf("md2h(38): not implemented\n");
        return 0;
    }

    FILE *f = fopen(argv[1], "rb");
    if (f == NULL) {
        perror("fopen");
        return 1;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        perror("fseek");
        fclose(f);
        return 1;
    }
    long size = ftell(f);
    if (size < 0) {
        perror("ftell");
        fclose(f);
        return 1;
    }
    rewind(f);

    char *src = malloc((size_t)size + 1);
    if (src == NULL) {
        perror("malloc");
        fclose(f);
        return 1;
    }

    size_t n = fread(src, 1, (size_t)size, f);
    if (n != (size_t)size) {
        perror("fread");
        free(src);
        fclose(f);
        return 1;
    }
    src[n] = '\0';
    fclose(f);

    md_out o = {0};
    mdparse(src, &o);

    if (o.err) {
        fprintf(stderr, "md2h(12): out of memory\n");
        free(o.buf);
        free(src);
        return 1;
    }

    FILE *out = stdout;
    if (argc >= 3 && argv[2] != NULL) {
        out = fopen(argv[2], "wb");
        if (out == NULL) {
            perror("fopen output");
            free(o.buf);
            free(src);
            return 1;
        }
    }

    size_t written = fwrite(o.buf, 1, o.len, out);
    if (written != o.len) {
        perror("fwrite");
        if (out != stdout) fclose(out);
        free(o.buf);
        free(src);
        return 1;
    }

    if (out != stdout) {
        if (fclose(out) != 0) {
            perror("fclose");
            free(o.buf);
            free(src);
            return 1;
        }
    }

    free(o.buf);
    free(src);
    return 0;
}

void cmd_help() {
    static const char usage[] = 
        "Usage: md2h [options] <target.md> <output.html>\n"
        " -V, --version     Show version number and quit\n"
        " -v, --verbose     Make the operation more talkative\n"
        " -h, --help        Show this text\n"
        "\n"
        "Have an issue? Report it here: https://github.com/z3nnix/md2H/issues\n";

    printf(usage);
}

void cmd_version() {
    printf("md2h 0.1\n");
}