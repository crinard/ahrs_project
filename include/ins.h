#ifndef INS_H
#define INS_H

#include <stdint.h>
#include "errors.h"

typedef struct INS_Data {
    uint8_t placeholder;
} INS_Data_t;

/**
 * @brief Initializes INS periphrial
 * @param hi2c I2c handle to use for INS communication
 * @return ERROR_OK if transmit function ok, ERROR_GENERIC otherwise
 */
error_t ins_init(void);

/**
 * @brief Reads INS data and saves into struct
 * @param data Struct to save data to.
 * @return ERROR_OK if transmit function ok, ERROR_GENERIC otherwise
 */
error_t ins_rx_cb(INS_Data_t *data);

#endif //INS_H