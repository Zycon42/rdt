/**
 * Projekt c.3 do predmetu IPK
 * @author Jan Dusek <xdusek17@stud.fit.vutbr.cz>
 * @date 17.4.2011
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "rdt.h"
#include "args.h"

#define MAX_LINE_SIZE 81

static const char *USAGE = "rdtclient -s SOURCE_PORT -d DEST_PORT";

int main(int argc, char **argv) {
    args_usage = USAGE;

    args_t args;
    if (parse_args(argc, argv, &args) == -1)
        return 1;

    rdt_t *rdt = rdt_create(args.source_port);
    if (rdt_connect(rdt, args.dest_port) == 0)
        fprintf(stderr, "Connection established\n");
    else {
        fprintf(stderr, "Connection failed\n");
        return 0;
    }

    if (rdt_send(rdt, stdin) == -1)
        fprintf(stderr, "Error in sending data from stream\n");

    rdt_close(rdt);

    return 0;
}
