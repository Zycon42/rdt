/**
 * Projekt c.3 do predmetu IPK
 * @author Jan Dusek <xdusek17@stud.fit.vutbr.cz>
 * @date 18.4.2011
 */

#include "args.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

const char* args_usage = "";

int parse_args(int argc, char** argv, args_t* args) {
    int opt;
    while ((opt = getopt(argc, argv, "s:d:")) != -1) {
        switch (opt) {
            case 's':
                args->source_port = atol(optarg);
                break;
            case 'd':
                args->dest_port = atol(optarg);
                break;
            default:
                fprintf(stderr, "%s\n", args_usage);
                return -1;
        }
    }
    if (optind < argc) {
        fprintf(stderr, "%s\n", args_usage);
        return -1;
    }

    return 0;
}
