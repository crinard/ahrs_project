#ifndef GDL_90_H
#define GDL_90_H
#include <stdint.h>
#include <string.h>

void gdl_90_init(void);
void crc_init(void);
void crc_inject(uint8_t *msg, size_t len);

#endif // GDL_90_h
