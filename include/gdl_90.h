#ifndef GDL_90_H
#define GDL_90_H

#include "ins.h"
#include "errors.h"

#define GDL_90_HEARTBEAT_MSG_LEN 7
#define GDL_90_ID_MSG_LEN 39

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#define NETWORK_NAME "CARTAGENA-H2"
#define NETWORK_PASSWORD  "Cartagena1914"
#define RX_PORT 63093
#define TX_PORT 4000

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

/**
 * @brief Updates recieving buffer
 * @return ERROR_OK if successful, ERROR_GENERIC otherwise.
 */
error_t GDL_90_rx_cb(void);
#endif //GDL_90_H
