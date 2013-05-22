/**
 * Projekt c.3 do predmetu IPK
 * @author Jan Dusek <xdusek17@stud.fit.vutbr.cz>
 * @date 18.4.2011
 */

#ifndef _RDT_H_
#define _RDT_H_

#include <stdio.h>
#include <netinet/in.h>

/// rdt connection descriptor
typedef struct _rdt rdt_t;

/**
 * Creates rdt connection binded on local port
 * @param local_port port which connection binds on
 * @return pointer to descriptor. NULL on error. Sets errno
 */
rdt_t * rdt_create(in_port_t local_port);

/**
 * Establishes connection to remote_port
 * @param rdt descriptor
 * @param remote_port port to connect to
 * @return 0 on success -1 on error. Sets errno
 */
int rdt_connect(rdt_t *rdt, in_port_t remote_port);

/**
 * Block caller till someone connects to local port
 * @param rdt descriptor
 * @param remote_port accept connection only from this port. 0 for all ports
 * @return 0 on success -1 on error. Sets errno
 */
int rdt_listen(rdt_t *rdt, in_port_t remote_port);

/**
 * Sends file stream over rdt connection. Lines max 80 chars long.
 * @param rdt descriptor
 * @param file stream opened for reading which will be sent
 * @return 0 on success -1 on error. Sets errno
 */
int rdt_send(rdt_t *rdt, FILE *file);

/**
 * Receive data from connection to file stream
 * @param rdt descriptor
 * @param file stream opened for writing
 * @return 0 on success or -1 if connection closed
 */
int rdt_recv(rdt_t *rdt, FILE *file);

/**
 * Closes rdt connection
 * @param rdt connection descriptor to close
 */
void rdt_close(rdt_t *rdt);

#endif // _RDT_H_
