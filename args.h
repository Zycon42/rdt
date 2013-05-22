/**
 * Projekt c.3 do predmetu IPK
 * @author Jan Dusek <xdusek17@stud.fit.vutbr.cz>
 * @date 18.4.2011
 */

#ifndef _ARGS_H_
#define _ARGS_H_

#include <netinet/in.h>

typedef struct {
    in_port_t dest_port;
    in_port_t source_port;
} args_t;

extern const char* args_usage;

int parse_args(int argc, char** argv, args_t* args);

#endif // _ARGS_H_
