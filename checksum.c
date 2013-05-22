/**
 * Projekt c.3 do predmetu IPK
 * @author Jan Dusek <xdusek17@stud.fit.vutbr.cz>
 * @date 18.4.2011
 */

#include "checksum.h"

uint16_t compute_checksum(const void* buf, size_t n) {
    uint16_t *ptr = (uint16_t *)buf;
    long sum = 0;
    while(n > 1) {
        sum += *ptr++;
        n -= 2;
    }

    // dodame byte aby jsme meli dvojci
    if(n > 0)
        sum += *(unsigned char *)ptr;

    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);

    return ~sum;
}
