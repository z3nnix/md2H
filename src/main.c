#include <stdio.h>
#include <string.h>
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
    
    return 0;
}

void cmd_help() {
    static const char usage[] = 
        "Usage: md2h [options] <target.md> <output.html>\n"
        " -V, --version     Show version number and quit\n"
        " -v, --verbose     Make the operation more talkative\n"
        " -s, --silent      Silent mode\n"
        " -h, --help        Show this text\n"
        "\n"
        "Have an issue? Report it here: https://github.com/z3nnix/md2H/issues\n";

    printf(usage);
}