#ifndef GDL_90_H
#define GDL_90_H

#include "ins.h"
#include "errors.h"

/**
 * @brief Initializes GDL_90 periphials and protocol
 * @return ERROR_OK if successful, ERROR_GENERIC otherwise
 */
error_t GDL_90_init(void);

/**
 * @brief Sends INS data in GDL90 format
 * @param data INS data to transmit
 * @return ERROR_OK if successful, ERROR_GENERIC otherwise.
 */
error_t GDL_90_tx_cb(INS_Data_t *data);

#endif //GDL_90_H
