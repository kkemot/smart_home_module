#pragma once

//Default settings
#define SSID_AP "**********"
#define PASS_AP "**********"
#define SSID_STA "**********"
#define PASS_STA "**********"
#define TS_WRITE_KEY "NO_API_KEY"
#define SLEEP_TIME_DEFAULT 60
#define WIFIMODE_DEFAULT 0 //0 - WIFI_AP_STA; 1 - WIFI_STA; 2 - WIFI_STA_DEEP_SLEEP; 3 - WIFI_OFF
#define TS_SERVER_NAME "api.thingspeak.com"
#define WIFIAP_TIMEOUT (60*3)

#define BUTTON_PRESSED  0
#define BUTTON_RELEASED 1

//Reserved pins:
//0 - Arduino pin: D3 (Flash button/BootMode)
//2 - Arduino pin: D4 (Buildin LED/BootMode)
//15 - Arduino pin: D8(BootMode)
//Normal mode: HHL
//Boot from uart: HLL
//
//16 - Arduino pin: D0 (wakeup <-> reset)

//Ports
#define USER_BUTTON 12  //Arduino pin: D6
#define LED_PIN     2   //Arduino pin: D4
#define LED_ON      LOW
#define LED_OFF     HIGH


//SHT1x sensor
#define sht_dataPin  5  //Arduino pin: D1
#define sht_clockPin 4  //Arduino pin: D2

//Dust sensor: sds011
#define dust_rx  14  //Arduino pin: D5
#define dust_tx  13  //Arduino pin: D7


