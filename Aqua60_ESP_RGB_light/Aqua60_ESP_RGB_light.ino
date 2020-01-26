#include <TimeLib.h>
#include <NtpClientLib.h>
#include <ESP8266WiFi.h>
#include <Wire.h>   // I2C ESP8266 - SDA - D4 / SCL - D5

#include <credentials.h>
const char *ssid     = WIFI_SSID;
const char *password = WIFI_PASSWD;
WiFiServer server(80);

#define ONBOARDLED 2 // Built in LED on ESP-12/ESP-07
#define I2CAddressESPWifi 8

int8_t timeZone = 1;
int8_t minutesTimeZone = 0;
bool wifiFirstConnected = false;

const byte numChars = 50;
char received_str[numChars]; // String for received commands from Serial
boolean newData = false; // Flag for new data on Serial
char arduino_time[19];
int red = 0;
int green = 0;
int blue = 0;
int white = 0;
int auto_mode = 0;
String bufer;
String i2cstr;


// WiFi Event Handlers
void onSTAConnected (WiFiEventStationModeConnected ipInfo) {
    //Serial.printf ("Connected to %s\r\n", ipInfo.ssid.c_str ());
}

// Start NTP only after IP network is connected
void onSTAGotIP (WiFiEventStationModeGotIP ipInfo) {
    //Serial.printf ("Got IP: %s\r\n", ipInfo.ip.toString ().c_str ());
    //Serial.printf ("Connected: %s\r\n", WiFi.status () == WL_CONNECTED ? "yes" : "no");
    digitalWrite (ONBOARDLED, LOW); // Turn on LED
    wifiFirstConnected = true;
}

// Manage network disconnection
void onSTADisconnected (WiFiEventStationModeDisconnected event_info) {
    //Serial.printf ("Disconnected from SSID: %s\n", event_info.ssid.c_str ());
    //Serial.printf ("Reason: %d\n", event_info.reason);
    digitalWrite (ONBOARDLED, HIGH); // Turn off LED
    //NTP.stop(); // NTP sync can be disabled to avoid sync errors
}

// NTP Sync Event
void processSyncEvent (NTPSyncEvent_t ntpEvent) {
    //if (ntpEvent) {
        //Serial.print ("Time Sync error: ");
        //if (ntpEvent == noResponse)
            //Serial.println ("NTP server not reachable");
            
        //else if (ntpEvent == invalidAddress)
            //Serial.println ("Invalid NTP server address");
    //} else {
        //Serial.print ("Got NTP time: ");
        //Serial.println (NTP.getTimeDateString (NTP.getLastNTPSync ()));
    //}
}


// Read Data from Serial
void receive_from_serial() {
    static byte ndx = 0;
    char endMarker = '\n';
    char rc;
 
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (rc != endMarker) {
            received_str[ndx] = rc;
            ndx++;
            if (ndx >= numChars) {
                ndx = numChars - 1;
            }
        }
        else {
            received_str[ndx] = '\0'; // terminate the string
            ndx = 0;
            newData = true;
       }
    }
}

// Parse data from Serial
void parse_from_serial() {

    if ( received_str[0] == 'S' ) {  // Status Message "S"<auto_mode><RED><GREEN><BLUE><WHITE> e.g. S0111222333444
        bufer = received_str;
        auto_mode = bufer.substring(1,2).toInt();
        red =   bufer.substring(2,5).toInt();
        green = bufer.substring(5,8).toInt();
        blue =  bufer.substring(8,11).toInt();
        white = bufer.substring(11,14).toInt();
    }
    
}

boolean syncEventTriggered = false; // True if a time even has been triggered
NTPSyncEvent_t ntpEvent; // Last triggered event

////////////////////////////////////////////////////////////
//   SETUP
////////////////////////////////////////////////////////////
void setup () {
    static WiFiEventHandler e1, e2, e3;

    Serial.begin (115200);
    WiFi.mode (WIFI_STA);
    WiFi.begin (ssid, password);

    // Setup I2C - ESP8266 - Master
    // SDA - D4 / SCL - D5
    Wire.begin(2,14);

    pinMode (ONBOARDLED, OUTPUT); // Onboard LED
    digitalWrite (ONBOARDLED, HIGH); // Switch off LED

    NTP.onNTPSyncEvent ([](NTPSyncEvent_t event) {
        ntpEvent = event;
        syncEventTriggered = true;
    });

    e1 = WiFi.onStationModeGotIP (onSTAGotIP);// As soon WiFi is connected, start NTP Client
    e2 = WiFi.onStationModeDisconnected (onSTADisconnected);
    e3 = WiFi.onStationModeConnected (onSTAConnected);


    // Start the server
    server.begin();
}

////////////////////////////////////////////////////////////
//   MAIN - LOOP
////////////////////////////////////////////////////////////
void loop () {
    //static int i = 0;
    static int last = 0;
    // NTP - get TIME 
    if (wifiFirstConnected) {
        wifiFirstConnected = false;
        NTP.begin ("pool.ntp.org", timeZone, true, minutesTimeZone);
        NTP.setInterval (63);
    }

    if (syncEventTriggered) {
        processSyncEvent (ntpEvent);
        syncEventTriggered = false;
    }


    if ((millis () - last) > 5100) {
        //Serial.println(millis() - last);
        last = millis ();
        //Serial.print (i); Serial.print (" ");
        Serial.println (NTP.getTimeDateString ());
        //Serial.print (NTP.isSummerTime () ? "Summer Time. " : "Winter Time. ");
        //Serial.print ("WiFi is ");
        //Serial.print (WiFi.isConnected () ? "connected" : "not connected"); Serial.print (". ");
        //Serial.print ("Uptime: ");
        //Serial.print (NTP.getUptimeString ()); Serial.print (" since ");
        //Serial.println (NTP.getTimeDateString (NTP.getFirstSync ()).c_str ());

        //i++;
    }


    WiFiClient client = server.available();
    if (!client) {
        return;
    }
    
    if ( client ) {

        Wire.beginTransmission(I2CAddressESPWifi);
        Wire.write("s");
        Wire.endTransmission();
        delay(1);//Wait for Slave to calculate response.
        Wire.requestFrom(I2CAddressESPWifi,numChars);
        byte ndx = 0;
        i2cstr = "";
        byte endtrans = 0;
        while (Wire.available()) {
          delay(1);
          char c = Wire.read();
          if ( char(c) == '|' ) {
            endtrans = 1;
          }
          if ( endtrans == 0 ) {
             received_str[ndx] = char(c);
             i2cstr += char(c);
             //Serial.print(c);
             //Serial.print('-');
             //Serial.println(char(c));
             newData = true;
             ndx++;
          }
        }
        received_str[ndx] = '\0'; // terminate the string
        Serial.println("<XX");
        Serial.print(i2cstr);
        Serial.print('=');
        Serial.print(received_str);
        Serial.println("<YY");
        //Serial.println("GS");     // Send Request for Status Data to ARDUINO
        //receive_from_serial();    // Receive Status Data from Serial ARDUINO
        if(newData) { parse_from_serial(); } // Parse Status Data

        // Wait until the client sends some data
        while(!client.available()){
            delay(1);
        }

        // Prepare the response
        String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nNTP: ";
        s += "M: " + String(auto_mode) + " R: " + String(red) + " G: " + String(green) + " B: " + String(blue) + " W: " + String(white);
        s += "</html>\n";
        // Send the response to the client
        client.print(s);
        client.flush();
        if (!client.connected()) {
            client.stop();
        }
        newData = false;
    }  
    delay (1);
}
