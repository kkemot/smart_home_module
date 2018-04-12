#include <ESP8266WiFi.h>
#include <SHT1x.h>
#include <SDS011.h>

#include "sensors.h"
#include "configuration.h"

SHT1x sht1x(sht_dataPin, sht_clockPin);
static float temperature;
static float humidity;


void init_sensors(void) {
}

void printSensorResoults(void) {
  char temp[10];
  int error;
  float pm25, pm10;
  // Read values from the sensor
  temperature = sht1x.readTemperatureC();
  humidity = sht1x.readHumidity();

  // Print the values to the serial port
  Serial.print("Temperature: ");
  sprintf(temp,"%.1f", temperature);
  Serial.print(temp);
  Serial.print("C / ");
  Serial.print(" Humidity: ");
  sprintf(temp,"%.0f", humidity);
  Serial.print(temp);
  Serial.println("%");

  if ((temperature < -100) || (temperature > 125) || (humidity < 0) || (humidity >100)) {
     Serial.println("Wrong sensor datas");
  }
  else {
     Serial.println("data ok");
  }
}

float getTemperature(void) {
  return sht1x.readTemperatureC();
}

float getHumidity(void) {
  return sht1x.readHumidity();
}

