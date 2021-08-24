#include <stdio.h>
#include "gdl_90.h"

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define NETWORK_NAME "CARTAGENA-H2"
#define NETWORK_PASSWORD  "Cartagena1914"

// buffers for receiving and sending data
static char m_packet_buffer[UDP_TX_PACKET_MAX_SIZE + 1]; //buffer to hold incoming packet,

static WiFiUDP m_udp;
IPAddress m_foreflight_address = IPAddress(192,168,0,103);
IPAddress m_generic_address = IPAddress(255,255,255,255);

static const char m_heartbeat_message_tx_buffer[GDL_90_HEARTBEAT_MSG_LEN] = {10, 0, 1, 0, 0, 0, 0};
static const char m_backwards_heartbeat_message_tx_buffer[7] = {0, 0, 0, 0, 1, 0, 10};
static const char m_id_message_tx_buffer[GDL_90_ID_MSG_LEN] = {0x65, 0, 1, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, //Serial Number
    0, 0, 0, 72, 69, 76, 76, 79, //Device Name,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 72, 69, 76, 76, 79, //Device long name
    0, 0, 0, 1 //Capabilities mask
    };
static const char m_backwards_id_message_tx_buffer[GDL_90_ID_MSG_LEN] = {1, 0, 0, 0, 
    79, 76, 76, 69, 72, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //Serial Number
    79, 76, 76, 69, 72, 76, 76, 79, //Device Name,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, //Device long name
    1, 0, 0x65 //Capabilities mask
    };

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
    return ERROR_OK;
};

error_t GDL_90_tx_cb(INS_Data_t *data) {
    //Send heartbeat message
    m_udp.beginPacket(m_foreflight_address, TX_PORT);
    for (uint8_t i = 0; i < sizeof(m_heartbeat_message_tx_buffer); i++) {
        m_udp.write(m_heartbeat_message_tx_buffer[i]);
    }
    m_udp.endPacket();

    //Send ID message
    m_udp.beginPacket(m_foreflight_address, TX_PORT);
    for (uint8_t i = 0; i < sizeof(m_id_message_tx_buffer); i++) {
        m_udp.write(m_id_message_tx_buffer[i]);
    }
    m_udp.endPacket();


    //Send backwards heartbeat message
    m_udp.beginPacket(m_foreflight_address, TX_PORT);
    for (uint8_t i = 0; i < sizeof(m_backwards_heartbeat_message_tx_buffer); i++) {
        m_udp.write(m_backwards_heartbeat_message_tx_buffer[i]);
    }
    m_udp.endPacket();

    //Send backwards ID message
    m_udp.beginPacket(m_foreflight_address, TX_PORT);
    for (uint8_t i = 0; i < sizeof(m_backwards_id_message_tx_buffer); i++) {
        m_udp.write(m_backwards_id_message_tx_buffer[i]);
    }
    m_udp.endPacket();


    //Send backwards heartbeat message
    m_udp.beginPacket(m_generic_address, TX_PORT);
    for (uint8_t i = 0; i < sizeof(m_backwards_heartbeat_message_tx_buffer); i++) {
        m_udp.write(m_backwards_heartbeat_message_tx_buffer[i]);
    }
    m_udp.endPacket();

    //Send backwards ID message
    m_udp.beginPacket(m_generic_address, TX_PORT);
    for (uint8_t i = 0; i < sizeof(m_backwards_id_message_tx_buffer); i++) {
        m_udp.write(m_backwards_id_message_tx_buffer[i]);
    }
    m_udp.endPacket();
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