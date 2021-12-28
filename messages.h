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

class ff_ahrs_msg {
    public:
        ff_ahrs_msg(void);
        ~ff_ahrs_msg(void);
        size_t buflen;
        uint8_t * buf;
};

#endif //MESSAGES_H
