#ifndef GDL_90_H
#define GDL_90_H

#include "ins.h"

typedef enum GDL_90_Error {
    GDL_ERROR_OK = 0,
    GDL_ERROR_GENERIC = 1,
} GDL_90_Error_t;

/**
 * @brief Initializes GDL_90 periphials and protocol
 * @return ERROR_OK if successful, ERROR_GENERIC otherwise
 */
GDL_90_Error_t GDL_90_init(void);

/**
 * @brief Sends INS data in GDL90 format
 * @param data INS data to transmit
 * @return ERROR_OK if successful, ERROR_GENERIC otherwise.
 */
GDL_90_Error_t GDL_90_tx_cb(INS_Data_t *data);

#endif //GDL_90_H
