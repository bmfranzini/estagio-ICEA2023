#ifdef __cplusplus
extern "C"
{
#endif
  float temprature_sens_read();
#ifdef __cplusplus
}
#endif
float temprature_sens_read();

#include <Arduino.h>
#include <WiFi.h>       // Biblioteca com implentação da Comunicação WIFI
#include "Ultrasonic.h" // Biblioteca para calculo de distancia com senosor de ultrason
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <iostream>
#include <TinyGPS++.h>
#include <sstream>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>          // include Arduino time library
#include <ThingSpeak.h>   // Biblioteca com implentação das chamadas ao sistemas ThingSpeaks

#define PIN_GPS_RX 16
#define PIN_GPS_TX 17
static const uint32_t GPSBaud = 9600;
HardwareSerial hs(1);
TinyGPSPlus gps;
WiFiClient client;

int intervalSensor = 2000;
long prevMillisThingSpeak = 0;
int intervalThingSpeak = 15000; // Intervalo minímo para escrever no ThingSpeak write é de 15 segundos

#define time_offset -10800  // define a clock offset of -10800 seconds (-3 hour) ==> UTC - 3
 
// variable definitions
char Time[]  = "00:00:00";
char Date[]  = "2000-00-00";
byte last_second, Second, Minute, Hour, Day, Month;
int Year;

void setup()
{
  Serial.begin(115200); // 115200);
  hs.begin(9600, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
  WiFi.mode(WIFI_STA); // Modo Station
  ThingSpeak.begin(client);  // Inicializa o ThingSpeak
  delay(2000);
}

const char ssid[] = "Pesquisa"; // Nome da Rede Wifi
const char password[] = "";     // Senha da Rede wifi

/***
 * Canal criado na Conta andreeliasmelo@gmail.com
 * Senha da Conta Ce-2892022
 * Channel ID: 2029105
 * Write Key KRLSAU5I1S5Q96O6
 * Read key YJBQ4MW95QUPV6JH
*/
const long CHANNEL = 2029105;
const char *WRITE_API = "KRLSAU5I1S5Q96O6";

const int PIN_SENSOR_ECHO = 2;         // Pino escolhido para leitura dos dados de Ultrason (distancia)
Ultrasonic ultrasonic(PIN_SENSOR_ECHO); // Chamada da Lib com os parametros de montagem

void print_speed()
{
  if (gps.location.isValid() == 1)
  {
    Serial.print("LAT:\n");
    float lat = gps.location.lat();
    Serial.print(lat);

    Serial.print("LNG:\n");
    float lng = gps.location.lng();
    Serial.print(lng);

    Serial.print("VEL:\n");
    float vel = gps.speed.kmph();
    Serial.print(vel);

    Serial.print("SATS:\n");
    float sats = gps.satellites.value();
    Serial.print(sats);

    Serial.print("ALT:\n");
    float alt = gps.altitude.meters();
    Serial.print(alt);
  }
  else
  {
    Serial.print("Dados nao obtidos\n");
  }
}

void loop()
{
  // Conecta ou reconecta o WiFi
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.printf("Atenção para conectar o SSID: ");
    Serial.println(ssid);
    while (WiFi.status() != WL_CONNECTED)
    {
      WiFi.begin(ssid, password);
      Serial.print(".");
      delay(2000);
    }
    Serial.println("\nConectado");
  }

  boolean newData = false;
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (hs.available())
    {
      if (gps.encode(hs.read()))
      {
        newData = true;
      }
    }
  }

  // If newData is true
  if (newData == true)
  {
    newData = false;
    Serial.println(gps.satellites.value());
    // print_speed();
  }
  else
  {
    Serial.print("SEM NOVOS DADOS\n");
  }

  HTTPClient http;
  Serial.println("Inicio do httpclient");
  http.begin("https://asa-homolog.decea.mil.br/api/position/uas/v1/uavs/telemetry");
  // http.begin("https://httpbin.org/post");

  http.addHeader("Content-Type", "text/json"); // Specify content-type header

  long distance = ultrasonic.MeasureInCentimeters();
  Serial.println("DISTANCIA: ");
  Serial.println(distance);

  if (distance > 50)
  {
    float lat = gps.location.lat();
    float lng = gps.location.lng();
    float alt = gps.altitude.meters();
    Serial.printf("LATITUDE: %f\n", lat);
    // Serial.printf("Latitude: %f  Longitude: %f\n", gps.location.lat(), gps.location.lng()); //, 1, 2);

    if (gps.time.isValid())
    {
      Minute = gps.time.minute();
      Second = gps.time.second();
      Hour = gps.time.hour();
    }

    // get date drom GPS module
    if (gps.date.isValid())
    {
      Day = gps.date.day();
      Month = gps.date.month();
      Year = gps.date.year();
    }

    if (last_second != gps.time.second()) // if time has changed
    {
      last_second = gps.time.second();

      // set current UTC time
      setTime(Hour, Minute, Second, Day, Month, Year);
      // add the offset to get local time
      adjustTime(time_offset);

      // update time array
      Time[6] = second() / 10 + '0';
      Time[7] = second() % 10 + '0';
      Time[3] = minute() / 10 + '0';
      Time[4] = minute() % 10 + '0';
      Time[0] = hour() / 10 + '0';
      Time[1] = hour() % 10 + '0';

      // update date array
      Date[2] = (year() / 10) % 10 + '0';
      Date[3] = year() % 10 + '0';
      Date[5] = month() / 10 + '0';
      Date[6] = month() % 10 + '0';
      Date[8] = day() / 10 + '0';
      Date[9] = day() % 10 + '0';

      std::string json1 = "{\"token\":\"D7KpdoUjemej8cdM1eX21enYnghJfqH7aYNOq00XIBgQwOCne3E2FiIbyXVQRKTlzeYJuPBzAdgwQdSHVyenJm5GpJRWVyRmmW8n2wVuLoG8eTKfcvtSZTFGVgQ44AqpyQsvxkISAFAwqByu52uHm7upO6iVxU5b\",\"telemetry\" : [{\"tailNumber\" : \"ICEA\" ,  \"position\" : {\"coordinates\" : {\"latitude\" : "; // -23.2083124,\"longitude\" : -45.8657117},\"AGL\" : -0.017  },  \"timestamp\" : \"2023-02-01T14:03:41-03:00\"}]}";
      std::string lat_str = std::to_string(lat);
      std::string json2 = ",\"longitude\" : ";
      std::string lng_str = std::to_string(lng);
      std::string json3 = "},\"AGL\" : -0.017  },  \"timestamp\" : \"";//2023-02-01T14:03:41-03:00\"}]}";
      std::string date_str = Date;
      std::string time_str = Time;
      std::string json4 = "\"}]}";

      std::string json = json1 + lat_str + json2 + lng_str + json3 + date_str + "T" + time_str + "-03:00" + json4;

      String v_play = String(json.c_str());
      int httpCode = http.POST(v_play);
      Serial.printf("httpCode : %d", httpCode);

      if (httpCode > 0)
      {
        // HThar json[] = "{\"sensor\":\"TP header has been send and Server response header has been handled
        Serial.printf("[HTTP] POST... code: %d\n", httpCode);
      }
      // file found at server
      if (httpCode == HTTP_CODE_OK)
      {
        String payload = http.getString();
        Serial.println(payload);
      }
      else
      {
        Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
    }
    http.end();

    //if (millis() - prevMillisThingSpeak > intervalThingSpeak) {
      
      delay(13000);
      ThingSpeak.setField(1, alt);
      ThingSpeak.setField(2, lat);
      ThingSpeak.setField(3, lng);
      ThingSpeak.setField(4, distance);
      
      // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
      // pieces of information in a channel.  Here, we write to field 1.
      int x = ThingSpeak.writeFields(CHANNEL, WRITE_API);
      if(x == 200){
      Serial.println("Channel update successful.");
      }
      else{
        Serial.println("Problem updating channel. HTTP error code " + String(x));
      }
    //}
    prevMillisThingSpeak = millis();
  }
}