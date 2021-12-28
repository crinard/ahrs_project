#include "messages.h"
#include "gdl_90.h"
#include <stdlib.h>
#include <string.h>

#define HEARTBEAT_MSG_ID 0
#define OWNSHIP_MSG_ID 10
#define TRAFFIC_MSG_ID 20
#define OWNSHIP_GEO_ALT_MSG_ID 11
#define UAT_UPLINK_MSG_ID 7
#define FF_AHRS_MSG_ID 0x65
#define FF_ID_MSG_ID 0x65

#define HEARTBEAT_MSG_LEN 11U
#define OWNSHIP_MSG_LEN 32U
#define TRAFFIC_MSG_LEN 32U
#define OWNSHIP_GEO_ALT_MSG_LEN 9U
#define UAT_UPLINK_MSG_LEN 440U
#define FF_AHRS_MSG_LEN 16U
#define FF_ID_MSG_LEN 43U

static uint8_t default_ahrs_msg[FF_AHRS_MSG_LEN] = {0x7E, 0x65, 0x01, //Message header
                                                0x7F, 0xFF, //Roll
                                                0x7F, 0xFF, //Pitch
                                                0x7F, 0xFF, //Heading
                                                0x7F, 0xFF, //Knots Indicated Airspeed
                                                0xFF, 0xFF, //Knots True Airspeed
                                                0, 0, 0x7E //CRC and end bit.
                                                };

ff_ahrs_msg::ff_ahrs_msg(void) {
    buflen = FF_AHRS_MSG_LEN;
    buf = (uint8_t *) malloc(FF_AHRS_MSG_LEN);
    memcpy(buf, default_ahrs_msg, FF_AHRS_MSG_LEN);
    crc_inject(buf, buflen);
    return;
}

ff_ahrs_msg::~ff_ahrs_msg(void) {
    free(buf); 
    return;
}
