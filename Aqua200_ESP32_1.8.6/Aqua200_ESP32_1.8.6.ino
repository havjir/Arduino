#define DEBUG 0

// Load Wi-Fi library
#include <WiFi.h>
#include "Wire.h"
#include <credentials.h>

// NTP - Time library
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <Time.h>
#include <Timezone.h>
#include <TimeLib.h>

TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, +120};
TimeChangeRule  CET = {"CET",  Last, Sun, Oct, 3, +60};

time_t local_time, utc;
Timezone czCET(CEST, CET);

char timestamp_str[16];
char buffer[100] = { 'Y','Y','Y','Y','/','M','M','/','D','D',' ','H','H',':','M','M',':','S','S','\0' };

struct tm timeinfo;

long NTPmillis_act = 0l;
long NTPmillis_last = 0l;
long rtcUpdateInterval = 10000; // RTC update from NTP interval in mili sec.

WiFiUDP ntpUDP;

// By default 'time.nist.gov' is used with 60 seconds update interval and no offset
// You can specify the time server pool and the offset, (in seconds)
// additionaly you can specify the update interval (in milliseconds).
// NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);
NTPClient timeClient(ntpUDP, "cz.pool.ntp.org", 0, 120000);


#include <ChainableLED.h>
// Library for RGB Strip driver P9813
/*
 * GND - GND
 * VCC - +5V
 * DIN - Data IN (serial)
 * CIN - Clock IN
 */
#define NUM_LEDS  1
#define LED_POS 0
#define DIN 18
#define CIN 19
ChainableLED leds(CIN, DIN, NUM_LEDS);

// Power Control variables
int red_power = 0;
int green_power = 0;
int blue_power = 0;
int white_power = 0;
boolean is_pwr_change = false;

// WEB Control variables
byte is_auto = 1;
int  web_pwr = 0;

// Replace with your network credentials
const char* ssid     = WIFI_SSID;
const char* password = WIFI_PASSWD;

// Wifi Connection Status LED
int ledConn = 32;

int ledMod = 2;

// Set web server port number to 80
WiFiServer server(80);

// Decode HTTP GET value
String redString = "0";
String greenString = "0";
String blueString = "0";
String whiteString = "0";
String autoString = "0";
int pos1 = 0;
int pos2 = 0;
int pos3 = 0;
int pos4 = 0;
int pos5 = 0;
int pos6 = 0;

// Variable to store the HTTP req  uest
String header;

// White LED pins for PWM control
const int whitePin = 21;     // 21 corresponds to GPIO13
long led_stat_millis_act = 0l;
long led_stat_millis_last = 0l;
byte led_stat = 0;


// Setting PWM frequency, channels and bit resolution
const int freq = 5000;
const int whiteChannel = 0;
// Bit resolution 2^8 = 256
const int resolution = 8;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;


// Program Steps of Auto Mode
#define  program_step_max  6

struct light_program {
  long sec_from;     // interval from - in sec from 00:00:00 
  long sec_to;       // interval to - in sec from 00:00:00 
  unsigned int dur;  // duration of power change (must be less than (sec_to - sec_from) - 0 = immediately
  byte tar_w;        // target power of white (0 - 255)
  byte tar_r;        // target power of red (0 - 255)
  byte tar_g;        // target power of green (0 - 255)
  byte tar_b;        // target power of blue (0 - 255)
};

light_program prog_step[program_step_max];

long current_sec = 0;




///////////////////////////////////////////
int validate_power(int vp_inppow){
  int vp_outpow = vp_inppow;
  if ( vp_outpow > 255 ) { vp_outpow = 255; }
  if ( vp_outpow < 0 ) { vp_outpow = 0; }
  is_pwr_change = true;
  
  return vp_outpow;
}

///////////////////////////////////////////
void set_auto_power(long sap_current_sec){
  int i = 0;
  byte int_num = 99;
  byte prev_step;
  int diff_w, diff_r, diff_g, diff_b = 0;
  //Serial.print("curent_sec: ");
  //sprintf(timestamp_str,"%d\n",sap_current_sec);
  //Serial.println(sap_current_sec);
  // Find valid program interval
  while (i < program_step_max) {
    if( sap_current_sec >= prog_step[i].sec_from && sap_current_sec <= prog_step[i].sec_to) {
      int_num = i;
    }
    i++;
  }

  // Find Previous Step
  if ( int_num == 0 ) {
    prev_step = program_step_max - 1;
  } else {
    prev_step = int_num - 1;
  }

  // Power Calculation
  if ( prog_step[int_num].dur == 0 || sap_current_sec > (prog_step[int_num].sec_from + prog_step[int_num].dur) ){  // Immediate Change (dur = 0)
    //Serial.println("I-CH");
    white_power = (byte)prog_step[int_num].tar_w;
    red_power = (byte)prog_step[int_num].tar_r;
    green_power = (byte)prog_step[int_num].tar_g;
    blue_power = (byte)prog_step[int_num].tar_b;
    
  } else {
    //Serial.println("P-C");
    diff_w = prog_step[int_num].tar_w - prog_step[prev_step].tar_w;
    diff_r = prog_step[int_num].tar_r - prog_step[prev_step].tar_r;
    diff_g = prog_step[int_num].tar_g - prog_step[prev_step].tar_g;
    diff_b = prog_step[int_num].tar_b - prog_step[prev_step].tar_b;
    //Serial.println(diff_w);
    white_power = validate_power(prog_step[prev_step].tar_w + (((float)diff_w / (float)prog_step[int_num].dur) * (sap_current_sec - prog_step[int_num].sec_from)));
    red_power   = validate_power(prog_step[prev_step].tar_r + (((float)diff_r / (float)prog_step[int_num].dur) * (sap_current_sec - prog_step[int_num].sec_from)));
    green_power = validate_power(prog_step[prev_step].tar_g + (((float)diff_g / (float)prog_step[int_num].dur) * (sap_current_sec - prog_step[int_num].sec_from)));
    blue_power  = validate_power(prog_step[prev_step].tar_b + (((float)diff_b / (float)prog_step[int_num].dur) * (sap_current_sec - prog_step[int_num].sec_from)));
  
  }

}

///////////////////////////////////////////////////////////////////////////////////
///   SETUP
///////////////////////////////////////////////////////////////////////////////////
void setup() {
  if(DEBUG) {Serial.begin(115200);}

  pinMode (ledConn, OUTPUT);
  pinMode (ledMod, OUTPUT);

  digitalWrite (ledMod, is_auto);
  
  // configure LED PWM functionalitites
  ledcSetup(whiteChannel, freq, resolution);
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(whitePin, whiteChannel);

  // Turn off RGB & White LED
  leds.setColorRGB(LED_POS, 0, 0, 0);
  ledcWrite(whiteChannel, 0);
  
  // WiFi Connection LED - Off
  digitalWrite (ledConn, led_stat);
  
  // Connect to Wi-Fi network with SSID and password
  if(DEBUG) {Serial.print("Connecting to ");}
  if(DEBUG) {Serial.println(ssid);}
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if(DEBUG) {Serial.print(".");}
  }
  // Print local IP address and start web server
  if(DEBUG) {Serial.println("");}
  if(DEBUG) {Serial.println("WiFi connected.");}
  if(DEBUG) {Serial.println("IP address: ");}
  if(DEBUG) {Serial.println(WiFi.localIP());}
  
  // Wifi Connection LED - On
  led_stat = 1;
  digitalWrite(ledConn, led_stat);
  
  server.begin();

  // Set RGB Strip Controler
  leds.init();
  leds.setColorRGB(LED_POS, 0, 0, 0);

  // Set NTP time
  timeClient.begin();
  //setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
  timeClient.update();
  if(DEBUG) {Serial.println("Waiting for NTP update");}
  while( timeClient.getEpochTime() < 946684800l ) {
    if(DEBUG) {Serial.print(".");}
    // LED bilnking
    led_stat_millis_act = millis();
    if((led_stat_millis_act - led_stat_millis_last) > 1000) {
      led_stat_millis_last = led_stat_millis_act;
      led_stat = (led_stat + 1) & 1;
    }
    digitalWrite(ledConn, led_stat);
    
    timeClient.update();
    utc = timeClient.getEpochTime();
    setTime(czCET.toLocal(utc));
  }
  if(DEBUG) {Serial.println("");}
  led_stat = 1;
  digitalWrite(ledConn, led_stat);

  // {<sec_from>,<sec_to>,<duration>,<WHITE>,<RED>,<GREEN>,<BLUE>}
  prog_step[0] = {0l,25200l,0,0,0,0,0};                 // 00:00:00 - 07:00:00 - light off (0%)
  prog_step[1] = {25201l,36000l,1800,255,255,255,255};  // 07:00:01 - 10:00:00 - 100%
  prog_step[2] = {36001l,54000l,600,0,100,60,60};       // 10:00:01 - 15:00:00 - 20-30%
  prog_step[3] = {54001l,72000l,1800,255,255,255,255};  // 14:00:01 - 20:00:00 - 100%
  prog_step[4] = {72001l,75600l,3500,0,70,30,50};     // 20:00:01 - 21:00:00 - 75%
  prog_step[5] = {75601l,86399l,900,0,0,0,0};           // 21:00:01 - 23:59:59 - light off (0%)

  
}

///////////////////////////////////////////////////////////////////////////////////
///   MAIN - LOOP
///////////////////////////////////////////////////////////////////////////////////
void loop(){

  // Reset NTCmillis - aftermillis overflow
  NTPmillis_act = millis();
  if(NTPmillis_last > NTPmillis_act) { NTPmillis_last = 0l; }
  
  if (WiFi.status() == WL_CONNECTED){
    digitalWrite (ledConn, HIGH);
    
    if((NTPmillis_act - NTPmillis_last) > rtcUpdateInterval){
       NTPmillis_last = NTPmillis_act;
       // Read RTCdata
       timeClient.update();
       if( timeClient.getEpochTime() > 946684800l ) {  // Check if updated time > 1.1.2000
          utc=timeClient.getEpochTime();
          setTime(czCET.toLocal(utc));
          if(DEBUG) {printf("%s\n", buffer);}
       }
    }
  } else {
    digitalWrite (ledConn, LOW);
  }

  if ( is_auto ) {
    current_sec = (hour() * (long)3600) + (minute() * (long)60) + (long)second();
    set_auto_power(current_sec);
    is_pwr_change = true;
  }
  
  WiFiClient client = server.available();   // Listen for incoming clients
  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    if(DEBUG) {Serial.println("New Client.");}          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {            // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        if(DEBUG) {Serial.write(c); }                   // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {

            // Request sample: /?r201g32b255w50&
            // Red = 201 | Green = 32 | Blue = 255 | White = 50
            if(header.indexOf("GET /?r") >= 0) {
              pos1 = header.indexOf('r');
              pos2 = header.indexOf('g');
              pos3 = header.indexOf('b');
              pos4 = header.indexOf('w');
              pos5 = header.indexOf('m');
              pos6 = header.indexOf('&');
              redString = header.substring(pos1+1, pos2);
              greenString = header.substring(pos2+1, pos3);
              blueString = header.substring(pos3+1, pos4);
              whiteString = header.substring(pos4+1, pos5);
              autoString = header.substring(pos5+1, pos6);
              /*Serial.println(redString.toInt());
              Serial.println(greenString.toInt());
              Serial.println(blueString.toInt());
              ledcWrite(redChannel, redString.toInt());
              ledcWrite(greenChannel, greenString.toInt());
              ledcWrite(blueChannel, blueString.toInt());*/

              red_power = redString.toInt();//2.56f;
              green_power = greenString.toInt();//2.56f;
              blue_power = blueString.toInt();//2.56f;
              white_power = whiteString.toInt();
              is_auto = autoString.toInt();

              if( !is_auto && red_power == 0 && green_power == 0 && blue_power == 0 && white_power == 0 ) { web_pwr = 0; }
              if( !is_auto && red_power == 25 && green_power == 25 && blue_power == 25 && white_power == 25 ) { web_pwr = 25; }
              if( !is_auto && red_power == 50 && green_power == 50 && blue_power == 50 && white_power == 50 ) { web_pwr = 50; }
              if( !is_auto && red_power == 75 && green_power == 75 && blue_power == 75 && white_power == 75 ) { web_pwr = 75; }
              if( !is_auto && red_power == 100 && green_power == 100 && blue_power == 100 && white_power == 100 ) { web_pwr = 100; }

              //if ( is_auto ) {
              //  current_sec = (hour() * (long)3600) + (minute() * (long)60) + (long)second();
              //  set_auto_power(current_sec);
              //  is_pwr_change = true;
              //}

            }
            
            // Actual time
            sprintf(buffer,"%d.%d.%d %02d:%02d:%02d",day(),month(),year(),hour(),minute(),second());

            
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
                   
            // Display the HTML web page

            client.println("<!DOCTYPE html><html>");
            client.println("<head>");
            client.println("   <meta name=\"viewport\" content=\"width=device-width\", initial-scale=\"1\">");
            client.println("   <link rel=\"icon\" href=\"data:,\">");
            client.println("   <link rel=\"stylesheet\" href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css\">");
            client.println("   <script src=\"https://cdnjs.cloudflare.com/ajax/libs/jscolor/2.0.4/jscolor.min.js\"></script>");
            client.println("</head>");
            // HTML Body
            client.println("<body>");
            client.println("   <div class=\"container\">");
            client.println("      <div class=\"row\">");
            client.println("         <h1>AKVARIUM 200 l</h1>");
            client.println("         <br><br><br>");
            client.println("      </div>");
              client.print("         <p>Time: ");
                                     client.print(buffer);
                                     client.println("<p/>");
            sprintf(buffer," W:%d R:%d G:%d B:%d ",white_power,red_power,green_power,blue_power);
              client.print("         <p>Power: ");
                                     client.print(buffer);
                                     client.println("<p/>");
            client.println("      </div>");
            // White Power - Slider
            client.println("      <div class=\"sliderW\">");
              client.print("         <input type=\"range\" class=\"slider\" id=\"myRange\" value=\"");
                                     client.print(white_power);
                                     client.println("\" >");
            client.println("         <a class=\"btn btn-primary btn-lg\" href=\"#\" id=\"SLD_W_00\" role=\"button\" onMouseDown=\"return handleMDown('buttonW')\">Change</a>");
            client.println("         <br><br>");
            client.println("      </div>");
            // RGB Collor Selector
            client.println("      <div class=\"collorSelector\">");
            client.println("         <input class=\"jscolor {onFineChange:'update(this)'}\" id=\"rgb\">");
            client.println("         <a class=\"btn btn-primary btn-lg\" href=\"#\" id=\"change_color\" role=\"button\">Change Color</a><p><p>");
            client.println("      </div>");
            // AUTO / MANUAL Mode Selector
            client.println("      <div class=\"modeSelector\">");
              client.print("         <input type=\"radio\" name=\"mode\" id = \"RBauto\" value=\"1\" ");
                                     if(is_auto) {client.print("checked");}
                                     client.println("> AUTO<br>");
              client.print("         <input type=\"radio\" name=\"mode\" id = \"RBoff\" value=\"0\" ");
                                     if(!is_auto && web_pwr == 0) {client.print("checked");}
                                     client.println("> OFF<br>");
              client.print("         <input type=\"radio\" name=\"mode\" id = \"RB25\" value=\"25\" ");
                                     if(!is_auto && web_pwr == 25) {client.print("checked");}
                                     client.println("> 25%<br>");
              client.print("         <input type=\"radio\" name=\"mode\" id = \"RB50\" value=\"50\" ");
                                     if(!is_auto && web_pwr == 50) {client.print("checked");}
                                     client.println("> 50%<br>");
              client.print("         <input type=\"radio\" name=\"mode\" id = \"RB75\" value=\"75\" ");
                                     if(!is_auto && web_pwr == 75) {client.print("checked");}
                                     client.println("> 75%<br>");
              client.print("         <input type=\"radio\" name=\"mode\" id = \"RB100\" value=\"100\" ");
                                     if(!is_auto && web_pwr == 100) {client.print("checked");}
                                     client.println("> 100%<br>");
            client.println("         <a class=\"btn btn-primary btn-lg\" href=\"#\" id=\"RB_M_00\" role=\"button\" onMouseDown=\"return handleMDown('buttonM')\">Submit</a>");
            //client.println("         <input type=\"submit\" href=\"#\" id=\"RAD_M_00\" onClick=\"radioSubmit()\" value=\"Submit\">");
            client.println("      </div>");
            client.println("   </div>");
            // Java Script
            client.println("   <script language=\"JavaScript\">");
            client.println("      var slider = document.getElementById(\"myRange\");");
            client.println("      var output = document.getElementById(\"demo\");");
            client.print("        var powerW = \"");
                                  client.print(white_power);
                                  client.println("\";");      
            client.print("        var powerR = \"");
                                  client.print(red_power);
                                  client.println("\";");      
            client.print("        var powerG = \"");
                                  client.print(green_power);
                                  client.println("\";");      
            client.print("        var powerB = \"");
                                  client.print(blue_power);
                                  client.println("\";");      
            client.print("        var autoMode = \"");
                                  client.print(is_auto);
                                  client.println("\";");      

            client.println("      function handleMDown(s_button) {");
            client.println("         if(s_button==\"buttonW\"){");
            client.println("            powerW = pad2(slider.value); autoMode = \"0\";");
            client.println("            document.getElementById(\"SLD_W_00\").href = \"?r\" + powerR + \"g\" + powerG + \"b\" + powerB + \"w\" + powerW + \"m\" + 0 + \"&\";");
            client.println("         }");
            client.println("         if(s_button==\"buttonM\"){");
            client.println("            if ( document.getElementById('RBauto').checked ){ autoMode = \"1\"; } else { autoMode = \"0\"; }");
            client.println("            if ( document.getElementById('RBoff').checked ){ autoMode = \"0\"; powerR = 0; powerG = 0; powerB = 0; powerW = 0;}");
            client.println("            if ( document.getElementById('RB25').checked ) { autoMode = \"0\"; powerR = 25; powerG = 25; powerB = 25; powerW = 25;}");
            client.println("            if ( document.getElementById('RB50').checked ) { autoMode = \"0\"; powerR = 50; powerG = 50; powerB = 50; powerW = 50;}");
            client.println("            if ( document.getElementById('RB75').checked ) { autoMode = \"0\"; powerR = 75; powerG = 75; powerB = 75; powerW = 75;}");
            client.println("            if ( document.getElementById('RB100').checked ) { autoMode = \"0\"; powerR = 100; powerG = 100; powerB = 100; powerW = 100;}");
            client.println("            document.getElementById(\"RB_M_00\").href = \"?r\" + powerR + \"g\" + powerG + \"b\" + powerB + \"w\" + powerW + \"m\" + autoMode + \"&\";");
            client.println("         }");
            client.println("      }");

            client.println("      slider.oninput = function() {powerW = this.value;}");

            client.println("      function pad2(number){");
            client.println("         return (number < 10 ? '0' : '') + number;");
            client.println("      }");

            client.println("      function update(picker) {");
            client.println("         powerR = Math.round(picker.rgb[0]); powerG = Math.round(picker.rgb[1]); powerB = Math.round(picker.rgb[2]); autoMode = \"0\";");
            client.println("         document.getElementById('rgb').innerHTML = Math.round(picker.rgb[0]) + ', ' +  Math.round(picker.rgb[1]) + ', ' + Math.round(picker.rgb[2]);");
            client.println("         document.getElementById( \"change_color\").href= \"?r\" + Math.round(picker.rgb[0]) + \"g\" + Math.round(picker.rgb[1]) + \"b\" + Math.round(picker.rgb[2]) + \"w\" + powerW + \"m0\" + \"&\";");
            client.println("      }");

            client.println("   </script>");
            client.println("</body>");
            client.println("</html>");
            

            // The HTTP response ends with another blank line
            client.println();

            
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    if(DEBUG) {Serial.println("Client disconnected.");}
    if(DEBUG) {Serial.println("");}
  }

  leds.setColorRGB(LED_POS, red_power, green_power, blue_power);
  ledcWrite(whiteChannel, white_power);
  digitalWrite (ledMod, is_auto);
    
  delay(20);
}
