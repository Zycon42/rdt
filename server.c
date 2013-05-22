/**
 * Projekt c.3 do predmetu IPK
 * @author Jan Dusek <xdusek17@stud.fit.vutbr.cz>
 * @date 18.4.2011
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "rdt.h"
#include "args.h"

#define MAX_LINE_SIZE 81

in_addr_t REMOTE_ADDR = 0x7f000001;
static const char *USAGE = "rdtserver -s SOURCE_PORT -d DEST_PORT";

int main(int argc, char **argv) {
    args_usage = USAGE;

    args_t args;
    if (parse_args(argc, argv, &args) == -1)
        return 1;

    rdt_t *rdt = rdt_create(args.source_port);
    rdt_listen(rdt, args.dest_port);
    fprintf(stderr, "Connection established\n");

    if (rdt_recv(rdt, stdout) == -1)
        fprintf(stderr, "Error recieving to stream\n");

    rdt_close(rdt);

    return 0;
}
