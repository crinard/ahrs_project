#ifndef MESSAGES_H
#define MESSAGES_H

#include <WiFiUdp.h>

typedef enum gdl_90_msg_type_e {
    HEARTBEAT,
    OWNSHIP,
    TRAFFIC,
    OWNSHIP_GEO_ALT,
    UAT_UPLINK,
    FF_AHRS,
    FF_ID,
} gdl_90_msg_type_e;

typedef enum altitude_type_e {
    WGS_84 = 0,
    MSL = 1,
} altitude_type_e;

class heartbeat_msg {
    public:
        heartbeat_msg(void);
        ~heartbeat_msg(void);
        uint8_t* buf;
        size_t buflen;
};

class ff_id_msg {
    public:
        ff_id_msg(const uint8_t short_name[8], const uint8_t long_name[16], uint64_t serial_number);
        ~ff_id_msg(void);
        uint8_t* buf;
        size_t buflen;
};

#endif //MESSAGES_H
