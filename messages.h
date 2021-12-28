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

//class heartbeat_msg {
//    public:
//        heartbeat_msg(void);
//        ~heartbeat_msg(void);
//        bool send(WiFiUDP buf);
//    private:
//        uint8_t* msg_buf;
//        size_t buflen;
//};
//
//class ff_id_msg {
//    public:
//        ff_id_msg(uint8_t short_name[8], uint8_t long_name[16], uint64_t serial_number);
//        ~ff_id_msg(void);
//        bool send(WiFiUDP buf);
//    private:
//        uint8_t* msg_buf;
//        size_t buflen;
//};

// class traffic_msg: public gdl_90_msg {
//     public:
//         traffic_msg(uint8_t callsign[8]);
//         set_alert_status(bool status);
//         set_lat_long(float latitude, float longitude);
//         set_altitude(uint32_t altitude);
//         set_ground_speed(uint16_t ground_speed);
//         set_vertical_speed(uint16_t vertical_speed);
//         set_heading(float heading);
//     private:
//         //TODO: there's some stuff left out here which should be filled in at some point.
//         bool traffic_alert_status;
//         uint8_t address_type;
//         uint8_t participant_address[3];
//         float latitude;
//         float longitude;
//         uint32_t altitude;
//         uint16_t ground_speed;
//         uint16_t vertical_speed;
//         float heading;
//         uint8_t callsign[8];
// }

// class ownship_msg: public traffic_msg { //Ownship is a special case of the traffic message.
//     public:
//     ownship_msg(void);
// };

class ff_ahrs_msg {
    public:
        ff_ahrs_msg(void);
        ~ff_ahrs_msg(void);
        void update_roll_pitch(float roll, float pitch);
        void update_heading(float hdg);
        bool send(WiFiUDP buf);
    private:
        size_t buflen; 
        float roll_deg, pitch_deg, heading_deg;
        uint16_t roll, pitch, yaw;
};

#endif //MESSAGES_H
