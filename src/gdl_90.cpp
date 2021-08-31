#include <stdio.h>
#include "gdl_90.h"

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define NETWORK_NAME "Airport Time Capsule"
#define NETWORK_PASSWORD  "addie123"

// buffers for receiving and sending data
static char m_packet_buffer[UDP_TX_PACKET_MAX_SIZE + 1]; //buffer to hold incoming packet,

static WiFiUDP m_udp;
static uint16_t Crc16Table[256] = {0};
IPAddress m_foreflight_address = IPAddress(192,168,2,143);
IPAddress m_generic_address = IPAddress(255,255,255,255);

static unsigned char m_heartbeat_message_tx_buffer[GDL_90_HEARTBEAT_MSG_LEN] = {0x7E, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x7E};
static unsigned char m_ownship_message_tx_buffer[GDL_90_OWNSHIP_MSG_LEN] = {0x7E, 10, 0x01, 
    0x00, 0x00, 0x00,
    0x00, 0x20, 0x00, 
    0x40, 0x00, 0x00,
    0x00, 0xF2, 0x88,
    0x00, 0x10, 0x01,
    0x02,
    0x01,
    0x4E, 0x38, 0x32, 0x35, 0x56, 0x20, 0x20, 0x20, 
    0x00,
    0, 0, 0x7E
};
static unsigned char m_id_message_tx_buffer[GDL_90_ID_MSG_LEN] = {0x7E, 0x65, 0, 1, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, //Serial Number
    0x4E, 0x38, 0x32, 0x35, 0x56, 0x20, 0x20, 0x20, //Device Name,
    0x4E, 0x38, 0x32, 0x35, 0x56, 0x20, 0x20, 0x20, 0x4E, 0x38, 0x32, 0x35, 0x56, 0x20, 0x20, 0x20, //Device long name
    0, 0, 0, 1, //Capabilities mask
    0, 0, 0x7E //CRC and end bit
};
// static unsigned char m_ahrs_message_tx_buffer[GDL_90_AHRS_MSG_LEN] = {0x7E, 0x65, 0x01, //Message header
//     0x00, 0x00, //Roll
//     0x00, 0x00, //Pitch
//     0x00, 0x00, //Heading
//     0x00, 0x00, //Knots Indicated Airspeed
//     0x00, 0x00, //Knots True Airspeed
//     0, 0, 0x7E //CRC and end bit.
// };

static void crc_init(void) {
    uint16_t i, bitctr, crc;
    for (i = 0; i < 256; i++)
    {
        crc = (i << 8);
        for (bitctr = 0; bitctr < 8; bitctr++)
        {
            crc = (crc << 1) ^ ((crc & 0x8000) ? 0x1021 : 0);
        }
        Crc16Table[i] = crc;
    }
}

static void crc_inject(unsigned char *msg, uint32_t length) {
    uint32_t i;
    uint16_t crc = 0;
    // Return – CRC of the block
    // i – Starting address of message
    // i – Length of message
    for (i = 1; i < length - 3; i++) {
        crc = Crc16Table[crc >> 8] ^ (crc << 8) ^ msg[i];
    }
    //Now we have the crc, put it in the bytes third from last and second from last
    msg[length - 3] = (uint8_t)(crc & 0x00FF);
    msg[length - 2] = (uint8_t)((crc & 0xFF00) >> 8);
}

error_t GDL_90_init(void) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(NETWORK_NAME, NETWORK_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(500);
    }
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());
    Serial.printf("UDP server on port %d\n", TX_PORT);
    m_udp.begin(RX_PORT);
    crc_init();
    return ERROR_OK;
};

error_t GDL_90_tx_cb(INS_Data_t *data) {
    crc_inject(&m_heartbeat_message_tx_buffer[0], sizeof(m_heartbeat_message_tx_buffer));
    crc_inject(&m_id_message_tx_buffer[0], sizeof(m_id_message_tx_buffer));
    crc_inject(&m_ahrs_message_tx_buffer[0], sizeof(m_ahrs_message_tx_buffer));
    crc_inject(&m_ownship_message_tx_buffer[0], sizeof(m_ownship_message_tx_buffer));

    Serial.print("Ownship message = ");
    //Send ownship message
    m_udp.beginPacket(m_foreflight_address, TX_PORT);
    for (uint8_t i = 0; i < sizeof(m_ownship_message_tx_buffer); i++) {
        m_udp.write(m_ownship_message_tx_buffer[i]);
        Serial.print(m_ownship_message_tx_buffer[i]);
        Serial.print(",");
    }
    m_udp.endPacket();
    Serial.println("\n");
    // Serial.print("Heartbeat message = ");
    // //Send heartbeat message
    // m_udp.beginPacket(m_generic_address, TX_PORT);
    // for (uint8_t i = 0; i < sizeof(m_heartbeat_message_tx_buffer); i++) {
    //     m_udp.write(m_heartbeat_message_tx_buffer[i]);
    //     Serial.print(m_heartbeat_message_tx_buffer[i]);
    //     Serial.print(",");
    // }
    // m_udp.endPacket();
    // Serial.println("\n");

    //Send ID message
    Serial.print("Id message = ");
    m_udp.beginPacket(m_generic_address, TX_PORT);
    for (uint8_t i = 0; i < sizeof(m_id_message_tx_buffer); i++) {
        m_udp.write(m_id_message_tx_buffer[i]);
        Serial.print(m_id_message_tx_buffer[i]);
        Serial.print(",");
    }
    Serial.println("\n");
    m_udp.endPacket();

    // //Send AHRS message
    // Serial.print("AHRS message = ");
    // m_udp.beginPacket(m_foreflight_address, TX_PORT);
    // for (uint8_t i = 0; i < sizeof(m_ahrs_message_tx_buffer); i++) {
    //     m_udp.write(m_ahrs_message_tx_buffer[i]);
    //     Serial.print(m_ahrs_message_tx_buffer[i]);
    //     Serial.print(",");
    // }
    // Serial.println("\n");
    // m_udp.endPacket();
    return ERROR_OK;
};

error_t GDL_90_rx_cb(void) {
    uint32_t packet_size = m_udp.parsePacket();
    while (packet_size) {
        // Serial.printf("Received packet of size %d from %s:%d\n    (to %s:%d, free heap = %d B)\n",
        //             packetSize,
        //             m_udp.remoteIP().toString().c_str(), m_udp.remotePort(),
        //             m_udp.destinationIP().toString().c_str(), m_udp.localPort(),
        //             ESP.getFreeHeap());

        // read the packet into packetBufffer
        uint32_t n = m_udp.read(m_packet_buffer, UDP_TX_PACKET_MAX_SIZE);
        m_packet_buffer[n] = 0;
        Serial.println("Contents:");
        Serial.println(m_packet_buffer);
    };
    return ERROR_OK;
};

