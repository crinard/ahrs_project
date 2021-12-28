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

static uint8_t default_heartbeat_msg[HEARTBEAT_MSG_LEN] = 
                                        {0x7E, 10,// Message header
                                         0xFF, 0x00, // Status bytes 1 & 2
                                         0x00, 0x00, // Timestamp
                                         0, 0, // Message counts
                                         0, 0, 0x7E //CRC and flag byte.
                                        };
static uint8_t default_id_msg[FF_ID_MSG_LEN] = 
                                        {0x7E, 0x65, 0, 1, //Message header
                                         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, //Serial number
                                         0x61, 0x68, 0x72, 0x73, 0x00, 0x00, 0x00, 0x00, //Device Name
                                         0x61, 0x68, 0x72, 0x73, 0x00, 0x00, 0x00, 0x00, //Device long name (16B)
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x01, //Capabilities mask
                                         0x00, 0x00, 0x7E //CRC and Flag byte
                                        };

heartbeat_msg::heartbeat_msg(void) {
    buflen = HEARTBEAT_MSG_LEN;
    buf = (uint8_t *) malloc(buflen);
    memcpy(buf, default_heartbeat_msg, buflen);
    crc_inject(buf, buflen); // Does not change after init, only calculate crc and end bytes once.
}

heartbeat_msg::~heartbeat_msg(void) {
    free(buf);
}

ff_id_msg::ff_id_msg(const uint8_t short_name[8], const uint8_t long_name[16], uint64_t serial_number) {
    buflen = FF_ID_MSG_LEN;
    buf = (uint8_t *) malloc(buflen); 
    memcpy(buf, default_id_msg, buflen);
    //Set serial number.
    for (size_t i = 0; i < sizeof(uint64_t); i++) {
        //little-big endian swap, write to the serial number indicies of the message buffer.
        buf[i + 4] = (serial_number >> (64 - (8 * i))) && 0xFF;
    }

    //Set short name
    memcpy(buf + 12, short_name, 8);
    //Set long name
    memcpy(buf + 20, long_name, 16);
    crc_inject(buf, buflen); // Does not change after init, only calculate crc and end bytes once.
}

ff_id_msg::~ff_id_msg(void) {
    free(buf);
}
