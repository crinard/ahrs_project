#include "messages.h"
#include "gdl_90.h"

#define HEARTBEAT_MSG_ID 0
#define OWNSHIP_MSG_ID 10
#define TRAFFIC_MSG_ID 20
#define OWNSHIP_GEO_ALT_MSG_ID 11
#define UAT_UPLINK_MSG_ID 7
#define FF_AHRS_MSG_ID 0x65
#define FF_ID_MSG_ID 0x65

#define HEARTBEAT_MSG_LEN 11
#define OWNSHIP_MSG_LEN 32
#define TRAFFIC_MSG_LEN 32
#define OWNSHIP_GEO_ALT_MSG_LEN 9
#define UAT_UPLINK_MSG_LEN 440
#define FF_AHRS_MSG_LEN 16
#define FF_ID_MSG_LEN 43

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
static uint8_t default_ahrs_msg[FF_AHRS_MSG_LEN] = {0x7E, 0x65, 0x01, //Message header
                                                0x7F, 0xFF, //Roll
                                                0x7F, 0xFF, //Pitch
                                                0x7F, 0xFF, //Heading
                                                0x7F, 0xFF, //Knots Indicated Airspeed
                                                0xFF, 0xFF, //Knots True Airspeed
                                                0, 0, 0x7E //CRC and end bit.
                                                };

heartbeat_msg::heartbeat_msg(void) {
    buflen = HEARTBEAT_MSG_LEN;
    msg_buf = (uint8_t *) malloc(buflen);
    memcpy(msg_buf, default_heartbeat_msg, buflen);
    crc_inject(msg_buf, buflen); // Does not change after init, only calculate crc and end bytes once.
}

heartbeat_msg::~heartbeat_msg(void) {
    free(msg_buf);
}

bool heartbeat_msg::send(WiFiUDP tx_buf) {
    return tx_buf.write(msg_buf, buflen);
}

ff_id_msg::ff_id_msg(uint8_t short_name[8], uint8_t long_name[16], uint64_t serial_number) {
    buflen = FF_ID_MSG_LEN;
    msg_buf = (uint8_t *) malloc(buflen); 
    memcpy(msg_buf, default_id_msg, buflen);
    //Set serial number.
    for (size_t i = 0; i < sizeof(uint64_t); i++) {
        //little-big endian swap, write to the serial number indicies of the message buffer.
        msg_buf[i + 4] = (serial_number >> (64 - (8 * i))) && 0xFF;
    }

    //Set short name
    memcpy(msg_buf + 12, short_name, 8);
    //Set long name
    memcpy(msg_buf + 20, long_name, 16);
    crc_inject(msg_buf, buflen); // Does not change after init, only calculate crc and end bytes once.
}

ff_id_msg::~ff_id_msg(void) {
    free(msg_buf);
}

bool ff_id_msg::send(WiFiUDP buf) {
    return tx_buf.write(msg_buf, buflen);
}

ff_ahrs_msg:ff_ahrs_msg(void) {
    buflen = FF_AHRS_MSG_LEN;
    msg_buf = (uint8_t *) malloc(buflen); 
    memcpy(msg_buf, default_ahrs_msg, FF_AHRS_MSG_LEN);
    crc_inject(msg_buf, buflen);
}

void ff_ahrs_msg::update_roll_pitch(float roll, float pitch) {
    roll_deg = roll;
    pitch_deg = pitch;
}

void ff_ahrs_msg::update_heading(float hdg) {
    heading_deg = hdg;
}

bool ff_ahrs_msg::send(WiFiUDP tx_buf) {
    // In messages that are not constant, need to update the fields and redo crc.

    return tx_buf.write(msg_buf, buflen);
}
