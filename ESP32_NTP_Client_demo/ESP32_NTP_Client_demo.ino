#include <NTPClient.h>
// change next line to use with another board/shield
//#include <ESP8266WiFi.h>
#include <WiFi.h> // for WiFi shield
//#include <WiFi101.h> // for WiFi 101 shield or MKR1000
#include <WiFiUdp.h>

#include <Time.h>
#include <Timezone.h>
#include <TimeLib.h>

TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, +120};
TimeChangeRule  CET = {"CET",  Last, Sun, Oct, 3, +60};

time_t local_time, utc;
Timezone czCET(CEST, CET);


#include <credentials.h>
const char *ssid     = WIFI_SSID;
const char *password = WIFI_PASSWD;

WiFiUDP ntpUDP;

// By default 'time.nist.gov' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP);

// You can specify the time server pool and the offset, (in seconds)
// additionaly you can specify the update interval (in milliseconds).
// NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

char timestamp_str[16];
char buffer[100];

struct tm timeinfo;

void setup(){
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  Serial.println("Waiting for WiFi connect");
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print(".");
  }
  Serial.println("");
  
  timeClient.begin();
  delay(5000);

  setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
  timeClient.update();
  Serial.println("Waiting for NTP update");
  while( timeClient.getEpochTime() < 946684800l ) {
    Serial.print(".");
    timeClient.update();
    utc = timeClient.getEpochTime();
    setTime(utc);
  }
  Serial.println("");

}

void loop() {
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  //Serial.print("timeClient Formated date: ");
  //Serial.println(timeClient.getFormattedDate());
  Serial.print("timeClient Formated time: ");
  Serial.println(timeClient.getFormattedTime());
  Serial.print("timeClient Epoch time: ");
  Serial.println(timeClient.getEpochTime());

  utc = timeClient.getEpochTime();
  localtime_r(&utc, &timeinfo);
  strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
  printf("%s\n", buffer);

  delay(2000);
}

