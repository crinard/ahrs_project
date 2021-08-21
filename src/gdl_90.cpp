#include "WiFiUdp.h"
#include "WiFiServer.h"
#include <stdio.h>
#include "gdl_90.h"

static WiFiUDP m_wifi_handle;

error_t GDL_90_init(void) {
    // m_wifi_handle = WiFiUDP();
    // m_wifi_handle.begin();
    // m_wifi_handle.
    return ERROR_OK;
};

error_t GDL_90_tx_cb(INS_Data_t *data) {
    return ERROR_OK;
};