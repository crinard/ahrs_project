#include "WiFiUdp.h"
#include "WiFiServer.h"
#include <stdio.h>
#include "gdl_90.h"

GDL_90_Error_t GDL_90_init(void) {
    WiFiUDP();
    return GDL_ERROR_OK;
};

GDL_90_Error_t GDL_90_tx_cb(INS_Data_t *data) {
    return GDL_ERROR_OK;
};