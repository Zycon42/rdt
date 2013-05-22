/**
 * Projekt c.3 do predmetu IPK
 * @author Jan Dusek <xdusek17@stud.fit.vutbr.cz>
 * @date 18.4.2011
 */

#ifndef _CHECKSUM_H_
#define _CHECKSUM_H_

#include <stdint.h>
#include <stdlib.h>

/**
 * Vypocita internet checksum
 * @param ptr data pro vypocet
 * @param n pocet bytu v data
 * @return internet checksum
 */
uint16_t compute_checksum(const void *buf, size_t n);

#endif  // _CHECKSUM_H_
