/*  AquaLIGHT
 *  LCD 16x2 I2C
 *  SCL - A5
 *  SDA - A4
 * 
 */

#include <Time.h>
#include <Timezone.h>

TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, +120};
TimeChangeRule  CET = {"CET",  Last, Sun, Oct, 3, +60};

time_t local_time, utc;
Timezone czCET(CEST, CET);

#include <TimeLib.h>

#include "Wire.h"

#define DS3231_I2C_ADDRESS 0x68

int RTC[7];

/*
// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val){
    return( (val/10*16) + (val%10) );
}
*/

// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val){
    return( (val/16*10) + (val%16) );
}

#include <IRLib.h>
// IR Receiver Module TSOP38238
#define MY_PROTOCOL NEC

#define BT_UP     0xFF3AC5 // Light up
#define BT_DW     0xFFBA45 // Light down

#define BT_PLAY   0xFF827D // Play button
#define BT_OFF    0xFF02FD // Off

#define BT_R      0xFF1AE5 // R button
#define BT_G      0xFF9A65 // G button
#define BT_B      0xFFA25D // B buton
#define BT_W      0xFF22DD // W buton

#define BT_C11    0xFF2AD5 // RED (row 1/ col 1)
#define BT_C12    0xFFAA55 // LIGHT GREEN (row 1/ col 2)
#define BT_C13    0xFF926D // DARK BLUE (row 1/ col 3)
#define BT_C14    0xFF12ED // PINK (row 1/ col 4)
#define BT_C21    0xFF0AF5 // ORANGE (row 2/ col 1)
#define BT_C22    0xFF8A75 // LIGHT BLUE (row 2/ col 2)
#define BT_C23    0xFFB24D // BROWN (row 2/ col 3)
#define BT_C24    0xFF32CD // PINK (row 2/ col 4)
#define BT_C31    0xFF38C7 // BEIGE bezova (row 3/ col 1)
#define BT_C32    0xFFB847 // MEDIUM BLUE (row 3/ col 2)
#define BT_C33    0xFF7887 // MEDIUM RED  (row 3/ col 3)
#define BT_C34    0xFFF807 // LIGHT BLUE  (row 3/ col 4)
#define BT_C41    0xFF18E7 // YELLOW (row 4/ col 1)
#define BT_C42    0xFF9867 // DARK BLUE (row 4/ col 2)
#define BT_C43    0xFF58A7 // RED (row 4/ col 3)
#define BT_C44    0xFFD827 // LIGHT BLUE (row 4/ col 4)

#define BT_RU     0xFF28D7 // RED UP
#define BT_GU     0xFFA857 // GREEN UP
#define BT_BU     0xFF6897 // BLUE UP
#define BT_RD     0xFF08F7 // RED DOWN
#define BT_GD     0xFF8877 // GREEN DOWN
#define BT_BD     0xFF48B7 // BLUE DOWN
#define BT_DIY1   0xFF30CF // DIY1
//#define BT_DIY2   0xFFB04F // DIY2
//#define BT_DIY3   0xFF708F // DIY3
#define BT_DIY4   0xFF10EF // DIY4
//#define BT_DIY5   0xFF906F // DIY5
//#define BT_DIY6   0xFF50AF // DIY6

#define BT_QUIC   0xFFE817 // QUICK
#define BT_SLOW   0xFFC837 // SLOW

#define BT_AUTO   0xFFF00F // AUTO
//#define BT_FLASH  0xFFD02F // FLASH
#define BT_FADE7  0xFFE01F // FADE7
//#define BT_FADE3  0xFF609F // FADE3
//#define BT_JUMP7  0xFFA05F // JUMP7
//#define BT_JUMP3  0xFF20DF // JUMP3

// LCD 16x2 I2C
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

#define I2C_ADDR    0x20 // <<----- Add your address here.  Find it from I2C Scanner
#define BACKLIGHT_PIN     3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7

LiquidCrystal_I2C  lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);


long Previous;   // Stores previous code to handle NEC repeat codes
String str_btn;

//Create a receiver object to listen on pin 11 - Timer2
IRrecv My_Receiver(11);
 
//Create a decoder object
IRdecode My_Decoder;

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
#define DIN 8
#define CIN 7

ChainableLED leds(CIN, DIN, NUM_LEDS);



byte white_pin = 9; // Pin for White Strip PWM control

byte autoLedPin = 13;  // LED for indication of AUTO/MANUAL mode

int i = 0;

int white_power = 0;
int red_power = 0;
int green_power = 0;
int blue_power = 0;
byte pow_step = 10;  // Power Step in manual mode 
boolean is_auto = true;
boolean is_pwr_change = false;
boolean is_lcd_on = false;
char timestamp_str[16];
char pwr_str[16];

char tmpstr[20] = "";
long lastMillisPWR = 0l;
long lastMillisTime = 0l;
long lastMillisRTC = 0;

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



///////////////////////////////////////////////////
// SETUP
///////////////////////////////////////////////////
void setup() {
  // put your setup code here, to run once:
  Wire.begin();
  Serial.begin(9600);

  pinMode(autoLedPin, OUTPUT);
  digitalWrite(autoLedPin, HIGH);
  
  My_Receiver.enableIRIn(); // Start the receiver
  
  pinMode(white_pin, OUTPUT);
  leds.init();
  leds.setColorRGB(LED_POS, 0, 0, 0);
  delay(30);

  lcd.begin (16,2); //  <<----- My LCD was 16x2
  lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);  // Switch on the backlight
  lcd.setBacklight(HIGH);
  lcd.home (); // go home
  lcd.setCursor (0,0);
  lcd.print(" LED  osvetleni ");  
  delay(2000);
  lcd.setBacklight(LOW);

  // setTime(hr,min,sec,day,month,year)
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  // retrieve data from DS3231
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  setTime(hour,minute,second,dayOfMonth,month,year);  
   // {<sec_from>,<sec_to>,<duration>,<WHITE>,<RED>,<GREEN>,<BLUE>}
  prog_step[0] = {0l,25200l,0,0,0,0,0};                 // 00:00:00 - 07:00:00 - light off (0%)
  prog_step[1] = {25201l,36000l,1800,255,255,255,255};  // 07:00:01 - 10:00:00 - 100%
  prog_step[2] = {36001l,54000l,600,0,100,40,40};       // 10:00:01 - 15:00:00 - 20-30%
  prog_step[3] = {54001l,72000l,1800,255,255,255,255};  // 14:00:01 - 20:00:00 - 100%
  prog_step[4] = {72001l,75600l,3500,0,150,80,100};     // 20:00:01 - 21:00:00 - 100%
  prog_step[5] = {75601l,86399l,900,0,0,0,0};           // 21:00:01 - 23:59:59 - light off (0%)
  
}

/*
//////////////////////////////////////////////////////
void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year){
    // sets time and date data to DS3231
    Wire.beginTransmission(DS3231_I2C_ADDRESS);
    Wire.write(0); // set next input to start at the seconds register
    Wire.write(decToBcd(second)); // set seconds
    Wire.write(decToBcd(minute)); // set minutes
    Wire.write(decToBcd(hour)); // set hours
    Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
    Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
    Wire.write(decToBcd(month)); // set month
    Wire.write(decToBcd(year)); // set year (0 to 99)
    Wire.endTransmission();
}
*/

/////////////////////////////////////////////////////////
void readDS3231time(byte *second, byte *minute, byte *hour, byte *dayOfWeek, byte *dayOfMonth, byte *month, byte *year){
    Wire.beginTransmission(DS3231_I2C_ADDRESS);
    Wire.write(0); // set DS3231 register pointer to 00h
    Wire.endTransmission();
    Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
    // request seven bytes of data from DS3231 starting from register 00h
    *second = bcdToDec(Wire.read() & 0x7f);
    *minute = bcdToDec(Wire.read());
    *hour = bcdToDec(Wire.read() & 0x3f);
    *dayOfWeek = bcdToDec(Wire.read());
    *dayOfMonth = bcdToDec(Wire.read());
    *month = bcdToDec(Wire.read());
    *year = bcdToDec(Wire.read());
}

////////////////////////////////////////////////////
void white_plus() {
  white_power = white_power + pow_step;
  white_power = validate_power(white_power);
  //sprintf(tmpstr,"WP: %d",white_power);
  //Serial.println(tmpstr);
}
void white_minus() {
  white_power = white_power - pow_step;
  white_power = validate_power(white_power);
  //sprintf(tmpstr,"WP: %d",white_power);
  //Serial.println(tmpstr);
}
void red_plus() {
  red_power = red_power + pow_step;
  red_power = validate_power(red_power);
  //sprintf(tmpstr,"RP: %d",red_power);
  //Serial.println(tmpstr);
}
void red_minus() {
  red_power = red_power - pow_step;
  red_power = validate_power(red_power);
  //sprintf(tmpstr,"RP: %d",red_power);
  //Serial.println(tmpstr);
}
void green_plus() {
  green_power = green_power + pow_step;
  green_power = validate_power(green_power);
  //sprintf(tmpstr,"GP: %d",green_power);
  //Serial.println(tmpstr);
}
void green_minus() {
  green_power = green_power - pow_step;
  green_power = validate_power(green_power);
  //sprintf(tmpstr,"GP: %d",green_power);
  //Serial.println(tmpstr);
}
void blue_plus() {
  blue_power = blue_power + pow_step;
  blue_power = validate_power(blue_power);
  //sprintf(tmpstr,"BP: %d",blue_power);
  //Serial.println(tmpstr);
}
void blue_minus() {
  blue_power = blue_power - pow_step;
  blue_power = validate_power(blue_power);
  //sprintf(tmpstr,"BP: %d",blue_power);
  //Serial.println(tmpstr);
}

///////////////////////////////////////////
void display_power() {

  char ctmp;
  if(is_auto){
    ctmp = 'A';
  }else{
    ctmp = 'M';
  }
  // Clear power line
  sprintf(pwr_str,"%s","                ");
  //delay(10);
  lcd.setCursor(0, 1);
  //delay(10);
  lcd.print(pwr_str);
  sprintf(pwr_str, "%3d|%3d|%3d|%3d%c", white_power, red_power, green_power, blue_power, ctmp);
  //lcd.home();
  delay(10);
  lcd.setCursor(0, 1);
  //delay(10);
  lcd.print(pwr_str);
  //delay(10);
  //Serial.println(pwr_str);
}

/*
void light_setup () {
  lcd.clear();
  lcd.setCursor ( 0, 0 );
  lcd.print("Setup - Mode");
  
}
*/

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

/////////////////////////////////////////////////////////////
// MAIN PROGRAM
/////////////////////////////////////////////////////////////
void loop() {
  // put your main code here, to run repeatedly:
  
  //Continuously look for results. When you have them pass them to the decoder
  if (My_Receiver.GetResults(&My_Decoder)) {
    My_Decoder.decode();    //Decode the data
    if(My_Decoder.decode_type==MY_PROTOCOL) {
    My_Decoder.DumpResults(); //Show the results on serial monitor
    if(My_Decoder.value==0xFFFFFFFF)
      My_Decoder.value=Previous;
      if (My_Decoder.value == BT_DIY4) { is_auto = false; white_minus(); }
      if (My_Decoder.value == BT_DIY1) { is_auto = false; white_plus();  }
      if (My_Decoder.value == BT_UP)   { is_auto = false; white_plus();red_plus();green_plus();blue_plus(); }
      if (My_Decoder.value == BT_DW)   { is_auto = false; white_minus();red_minus();green_minus();blue_minus(); }
      if (My_Decoder.value == BT_RD) { is_auto = false; red_minus(); }
      if (My_Decoder.value == BT_RU) { is_auto = false; red_plus(); }
      if (My_Decoder.value == BT_GD) { is_auto = false; green_minus(); }
      if (My_Decoder.value == BT_GU) { is_auto = false; green_plus(); }
      if (My_Decoder.value == BT_BD) { is_auto = false; blue_minus(); }
      if (My_Decoder.value == BT_BU) { is_auto = false; blue_plus(); }
      if (My_Decoder.value == BT_R) { is_auto = false; white_power = 0; red_power = 255; green_power = 0; blue_power = 0; is_pwr_change = true; }
      if (My_Decoder.value == BT_G) { is_auto = false; white_power = 0; red_power = 0; green_power = 255; blue_power = 0; is_pwr_change = true; }
      if (My_Decoder.value == BT_B) { is_auto = false; white_power = 0; red_power = 0; green_power = 0; blue_power = 255; is_pwr_change = true; }
      if (My_Decoder.value == BT_W) { is_auto = false; white_power = 255; red_power = 0; green_power = 0; blue_power = 0; is_pwr_change = true; }
      if (My_Decoder.value == BT_C11) { is_auto = false; white_power = 0; red_power = 255; green_power = 65; blue_power = 0; is_pwr_change = true; }
      if (My_Decoder.value == BT_C12) { is_auto = false; white_power = 0; red_power = 154; green_power = 205; blue_power = 50; is_pwr_change = true; }
      if (My_Decoder.value == BT_C13) { is_auto = false; white_power = 0; red_power = 65; green_power = 105; blue_power = 225; is_pwr_change = true; }
      if (My_Decoder.value == BT_C14) { is_auto = false; white_power = 0; red_power = 255; green_power = 182; blue_power = 193; is_pwr_change = true; }
      if (My_Decoder.value == BT_C21) { is_auto = false; white_power = 0; red_power = 255; green_power = 99; blue_power = 71; is_pwr_change = true; }
      if (My_Decoder.value == BT_C22) { is_auto = false; white_power = 0; red_power = 135; green_power = 206; blue_power = 250; is_pwr_change = true; }
      if (My_Decoder.value == BT_C23) { is_auto = false; white_power = 0; red_power = 165; green_power = 42; blue_power = 42; is_pwr_change = true; }
      if (My_Decoder.value == BT_C24) { is_auto = false; white_power = 0; red_power = 219; green_power = 112; blue_power = 147; is_pwr_change = true; }
      if (My_Decoder.value == BT_C31) { is_auto = false; white_power = 0; red_power = 255; green_power = 140; blue_power = 0; is_pwr_change = true; }
      if (My_Decoder.value == BT_C32) { is_auto = false; white_power = 0; red_power = 100; green_power = 149; blue_power = 237; is_pwr_change = true; }
      if (My_Decoder.value == BT_C33) { is_auto = false; white_power = 0; red_power = 176; green_power = 48; blue_power = 96; is_pwr_change = true; }
      if (My_Decoder.value == BT_C34) { is_auto = false; white_power = 0; red_power = 0; green_power = 255; blue_power = 255; is_pwr_change = true; }
      if (My_Decoder.value == BT_C41) { is_auto = false; white_power = 0; red_power = 255; green_power = 255; blue_power = 0; is_pwr_change = true; }
      if (My_Decoder.value == BT_C42) { is_auto = false; white_power = 0; red_power = 65; green_power = 105; blue_power = 225; is_pwr_change = true; }
      if (My_Decoder.value == BT_C43) { is_auto = false; white_power = 0; red_power = 255; green_power = 20; blue_power = 147; is_pwr_change = true; }
      if (My_Decoder.value == BT_C44) { is_auto = false; white_power = 0; red_power = 176; green_power = 224; blue_power = 230; is_pwr_change = true; } 
      if (My_Decoder.value == BT_SLOW) { pow_step = 10; }
      if (My_Decoder.value == BT_QUIC) { pow_step = 50; }
      if (My_Decoder.value == BT_OFF) { is_auto = false; white_power = 0; red_power = 0; green_power = 0; blue_power = 0; is_pwr_change = true; }
      if (My_Decoder.value == BT_PLAY) { is_auto = false; white_power = 255; red_power = 255; green_power = 255; blue_power = 255; is_pwr_change = true; }
      if (My_Decoder.value == BT_AUTO) { if(is_auto){is_auto = false;}else{is_auto = true;}; is_pwr_change = true; }
      if (My_Decoder.value == BT_FADE7) { if(is_lcd_on){lcd.setBacklight(LOW);is_lcd_on = false;}else{lcd.setBacklight(HIGH);is_lcd_on = true;} }
    }
    My_Receiver.resume();     //Restart the receiver

  }

  if(is_auto){
    digitalWrite(autoLedPin, HIGH);
  }else{
    digitalWrite(autoLedPin, LOW);
  }

  //sprintf(pwr_str, "X:%3d|%3d|%3d|%3d%", white_power, red_power, green_power, blue_power);
  //Serial.println(pwr_str);
  leds.setColorRGB(LED_POS, red_power, green_power, blue_power);
  analogWrite(white_pin, white_power);

  // Preteceni counteru millis() po 49,7 dnech
  if ( millis() < lastMillisPWR ) {
    lastMillisPWR = 0l;
  }
  if ( millis() < lastMillisRTC ) {
    lastMillisRTC = 0l;
  }
  if ( millis() < lastMillisTime ) {
    lastMillisTime = 0l;
  }

  if ( is_pwr_change && (millis() > (lastMillisPWR + 300)) ){
    lastMillisPWR = millis();
    display_power();
    is_pwr_change = false;
  }

  // Every 600 second setup internal Arduino clock from RTC
  if ( millis() > (lastMillisRTC + 600000) ){
    lastMillisRTC = millis();
    byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
    // retrieve data from DS3231
    readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
    setTime(hour,minute,second,dayOfMonth,month,year);
  }

  if ( millis() > (lastMillisTime + 1000) ){
    lastMillisTime = millis();
    utc = now();
    local_time = czCET.toLocal(utc); 
    //digitalClockDisplay();
    sprintf(timestamp_str, "%02d.%02d. %02d:%02d:%02d ",day(local_time),month(local_time),hour(local_time),minute(local_time),second(local_time));
    //delay(10);
    lcd.setCursor(0, 0);
    //delay(10);
    lcd.print(timestamp_str);
    //delay(20);
    //Serial.println(timestamp_str);
  }

  if ( is_auto ) {
    utc = now();
    local_time = czCET.toLocal(utc); 
    current_sec = (hour(local_time) * (long)3600) + (minute(local_time) * (long)60) + (long)second(local_time);
    //Serial.print("curent_sec: ");
    //sprintf(timestamp_str,"%d - %d - %d\n",hour(local_time),minute(local_time),second(local_time));
    //Serial.println(timestamp_str);
    set_auto_power(current_sec);
    is_pwr_change = true;
  }
  
  delay(80);
}
