#include <Wire.h>
#include <SoftwareSerial.h>

boolean DEBUG = false;

char debug_str[255];

// -- SETING VARIABLES -- //
int read_hum_interval = 900; // Read Humidity interval [sec]

int watering_time [] = {700, 730, 830, 1800, 1830, 1900}; // watering time HHMM

int max_watering_duration = 360;  // Maximal watering time [sec]

int max_hum = 80;   // Optimal humidity [%] - No watering
int half_hum = 65;  // Medium humidity  [%] - Half watering
int low_hum = 55;   // Low humidity     [%] - Full watering

double min_water_level = 8;  // Minimum level in water tank [cm]
double water_tank_high = 43;  // The high of water tank from bottom to the sensor [cm]


#define DS3231_I2C_ADDRESS 0x68

// ESP8266 - inicializace ser. portu na 10 a 11 pinu
//SoftwareSerial(RX, TX);
// Arduino 11 -> ESP-TX
// Arduino 10 -> ESP-RX
SoftwareSerial ESP =  SoftwareSerial(11, 10);

// Ultrasonic sensor HC-SR04
#define echoPin 47 //2 // Echo Pin
#define trigPin 49 //3 // Trigger Pin

// Sensor Connection:
//   Sensor #0 (D2,D3,A0)
//   Sensor #1 (D4,D5,A1)
//   Sensor #2 (D6,D7,A2)
int voltagePin_a[] = {8,4,6};
int voltagePin_b[] = {9,5,7};
char analogPin[] = {A0,A1,A2};

// PUMP control pin
// PUMP_1 = D10
// PUMP_2 = D11
int  pump_control_pin[] = {31, 33, 35};


int sensorValue[] = {0,0,0};

int flipTimer = 1000;

float hum[] = {0.00, 0.00, 0.00};

int RTC[7];

long lastMillisHum = 0l;
long lastMillisRTC = 0l;

byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
char timestamp_str[35];
char hum_str[30];
char log_str[50];
String esp_str;
String esp_hum_all_str;
String esp_wtr_all_str;
String esp_inpstr;
byte watering_flg = 0;
int  last_warering_time = 0; // last watering time [min] from 00:00
int  watering_duration [] = {0, 0, 0};
char watering_duration_str[10];
int water_level = -1;
char water_level_str[10];
int  watering_in_progress = 0;
long act_millis = 0l;
long last_pump_millis[] = {0l, 0l, 0l};
long  pump_watering_duration[] = {0l, 0l, 0l};
int  pump_status = 0;

//========================================================================
//== SETUP ==
//========================================================================
void setup() {
  // put your setup code here, to run once:
  Wire.begin();
  Serial.begin(9600);
  delay(500);
  Serial.println("Setup");
  
  ESP.begin(9600); // inicializace ESP-seriové linky
  delay(500);

  // Initialize Sensor Pins
  int i = 0;
  while ( i <= 2 ) {
    pinMode(voltagePin_a[i], OUTPUT);
    pinMode(voltagePin_b[i], OUTPUT);
    pinMode(analogPin[i], INPUT);
    i++;
  }

  // Initialize PUMP control pins
  i = 0;
  while ( i <= 2 ) {
    pinMode(pump_control_pin[i], OUTPUT);
    i++;
  }
  

  // Initialize Ultrasonic sensor HC-SR04
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);


  Serial.println("End Setup");
}

//== Convert normal decimal numbers to binary coded decimal ==============
byte decToBcd(byte val){
    return( (val/10*16) + (val%10) );
}

//== Convert binary coded decimal to normal decimal numbers ===============
byte bcdToDec(byte val){
    return( (val/16*10) + (val%16) );
}

//== SET DST TIME =============================================================
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


//== READ DST TIME ============================================================
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


//== READ HUMIDITY ===================================
float readHumidity(int sensorNum){

  if(DEBUG) {Serial.println("readHumidity - start");}
  
  int val1 = 0;
  int val2 = 0;
  float avg = 0.0;
  float hum = 0.0;
  
  // First measurement
  setPolarity(sensorNum, true);
  delay(flipTimer);
  val1 = analogRead(analogPin[sensorNum]);

  // Change polarity - second measurement
  setPolarity(sensorNum, false);
  delay(flipTimer);
  val2 = 1023 - analogRead(analogPin[sensorNum]);

  // End measurement - LOW
  toGND(sensorNum);
  
  avg = ((float)val1 + (float)val2) / 2.0;

  hum = 100 - (avg / 10.23);

  if(DEBUG) {Serial.println("readHumidity - stop");}
  return hum;
}

//== SET POLARITY ======================================
void setPolarity(int sensorNum, boolean flip){
 if(flip){
  digitalWrite(voltagePin_a[sensorNum], HIGH);
  digitalWrite(voltagePin_b[sensorNum], LOW);
 }
 else{
  digitalWrite(voltagePin_a[sensorNum], LOW);
  digitalWrite(voltagePin_b[sensorNum], HIGH);
 }
}

//== toGND ==============================================
void toGND(int sensorNum) {
  digitalWrite(voltagePin_a[sensorNum], LOW);
  digitalWrite(voltagePin_b[sensorNum], LOW);
}

//== CHECK WATERING TIME ====================================
byte checkWateringTime(int chwt_lastWatering) {

  if(DEBUG) {Serial.println("CHWT - checkWateringTime - start");}
  
  byte chwt_out = 0;
  int  chwt_actMin = (hour * 60) + minute;
  int i = 0;
  int chwt_wateringTime;
  int imax = sizeof(watering_time) / sizeof(int);
  if(DEBUG) {Serial.print("CHWT Last watering time[minutes]: ");}
  if(DEBUG) {Serial.println(chwt_lastWatering);}
  if(DEBUG) {Serial.print("CHWT Acual time[minutes]: ");}
  if(DEBUG) {Serial.println(chwt_actMin);}

  while (i < imax){
    chwt_wateringTime = ((watering_time[i]/100) * 60) + (watering_time[i] - ((watering_time[i]/100)*100));
    if(DEBUG) {Serial.print("CHWT Watering time[");}
    if(DEBUG) {Serial.print(i);}
    if(DEBUG) {Serial.print("]: ");}
    if(DEBUG) {Serial.println(chwt_wateringTime);}
    if( (chwt_actMin >= chwt_wateringTime) && 
        (chwt_actMin <= (chwt_wateringTime + (max_watering_duration / 60))) &&
        ((chwt_lastWatering < (chwt_actMin - 5)) || (chwt_lastWatering > (chwt_actMin + 5)))) {
      chwt_out = i + 1; 
    }
    i++;
  }

  if(DEBUG) {Serial.print("CHWT Is watering time: ");}
  if(DEBUG) {Serial.println(chwt_out);}
  if(DEBUG) {Serial.println("CHWT - checkWateringTime - end");}
  return chwt_out;
}

//== GET WATER LEVEL =========================================
int getWaterLevel() {

  if(DEBUG) {Serial.println("getWaterLevel");}
  
  int wl_i = 0 , wl_j = 0;
  float wl_level;
  int wl_maximumRange = 100; // Water Level Maximum range needed
  int wl_minimumRange = 20; // Water Level Minimum range needed
  int wl_duration, wl_tmp_dist, wl_distance[5]; // Water Level Duration used to calculate distance
  
  while ( wl_i < 5 && wl_j < 50) {
    delay(100);
    digitalWrite(trigPin, LOW); 
    delayMicroseconds(2); 

    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10); 
 
    digitalWrite(trigPin, LOW);
    wl_duration = pulseIn(echoPin, HIGH);
 
    //Calculate the distance (in cm) based on the speed of sound.
    wl_tmp_dist = wl_duration / 58.2;

    if ( wl_tmp_dist >= wl_minimumRange && wl_tmp_dist <= wl_maximumRange ) {
      wl_distance[wl_i] = wl_tmp_dist;
      wl_i++;
    } else {
      wl_j++;
    }
    delay(10);
  }
  //char wl_tmp_str[100];
  //sprintf(wl_tmp_str, "Sonic Dist: %d|%d|%d|%d|%d",wl_distance[0],wl_distance[1],wl_distance[2],wl_distance[3],wl_distance[4]);
  //Serial.println(wl_tmp_str);
  if ( wl_j < 50 ) {
    wl_i = 0;
    wl_tmp_dist = 0;
    while ( wl_i < 5 ) {
      wl_tmp_dist = wl_tmp_dist + wl_distance[wl_i];
      wl_i++;
    }
    wl_level = water_tank_high - (wl_tmp_dist / 5.0);
  } else {
    wl_level = 999;
  }

  if(DEBUG) {Serial.println("getWaterLevel - end");}
  return (int)wl_level;
}

//== CHECK INPUT ESP COMMAND =========================================
void checkEspCommand() {
  if(DEBUG) {Serial.println("CHECK INPUT ESP COMMAND - start");}
  Serial.println(esp_inpstr);

  if (esp_inpstr.startsWith("T")){  // Time synchronization
    // byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year
    byte st_year = esp_inpstr.substring(4,5).toInt();
    byte st_month = esp_inpstr.substring(6,7).toInt();
    byte st_dayOfMonth = esp_inpstr.substring(7,8).toInt();
    byte st_hour = esp_inpstr.substring(9,10).toInt();
    byte st_minute = esp_inpstr.substring(11,12).toInt();
    byte st_second = esp_inpstr.substring(13,14).toInt();
    byte st_dayOfWeek = esp_inpstr.substring(16,17).toInt() + 1;
    
    char st_tmp_str[100];
    sprintf(st_tmp_str, "Parsed TIME: %d-%d-%d#%d:%d:%d-%d",st_year, st_month, st_dayOfMonth, st_hour, st_minute, st_second, st_dayOfWeek);
    Serial.println(st_tmp_str);
    //setDS3231time(st_second, st_minute, st_hour, st_dayOfWeek, st_dayOfMonth, st_month, st_year);
  }


  if(DEBUG) {Serial.println("CHECK INPUT ESP COMMAND - end");}  
}

//== READ INPUT ESP COMMAND =========================================
void readEspCommand() {
  if(DEBUG) {Serial.println("READ INPUT ESP COMMAND - start");}

  if(DEBUG){Serial.println("Read ESP");}
  char znak = 'x';
  esp_inpstr = "";
  int tmpcnt = 0;
  boolean is_cmd = false;
  if ( ESP.available() ) {
     if(DEBUG){Serial.println("Read ESP - available");}
     while ( znak != '\n' && tmpcnt < 200) {
        znak = ESP.read();
        //Serial.println("ESP-write");
        //Serial.println(znak);
        if ( znak == '\n' ) {
           is_cmd = true;
        } else {
           esp_inpstr = esp_inpstr + (String)znak;
        }
        tmpcnt++;
     }
     
     if( tmpcnt >= 200 ) {
        // Clear buffer
        while (ESP.read() > 0) {delay(1);}  /* do nothing else */
        Serial.println("ESP ERROR Read cmd");
        Serial.println(esp_inpstr);
        esp_inpstr = "";
        is_cmd = false;
     }
     
     if ( is_cmd ){
        checkEspCommand();
        is_cmd = false;
     }
  }

  if(DEBUG) {Serial.println("READ INPUT ESP COMMAND - end");}
}

//============================================================
//  MAIN PROGRAM
//============================================================
void loop() {
  // put your main code here, to run repeatedly:

  if(DEBUG) {Serial.println("main program1 - start");}
  
  int i;

  // Preteceni counteru millis() po 49,7 dnech
  if ( millis() < lastMillisHum ) { // Preteceni counteru mereni vlhkosti
    lastMillisHum = 0l;
  }
  if ( millis() < lastMillisRTC ) { // Preteceni counteru pro RTC
    lastMillisRTC = 0l;
  }

  // Read RTC - 300 ms interval
  if ( millis() >= (lastMillisRTC + 300l) ) {

    if(DEBUG) {Serial.println("main2 - read RTC");}
    lastMillisRTC = millis();
    //Serial.println("DEBUG: Ctu RTC");

    // retrieve data from DS3231
    readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  }

  // Check Watering time
  if(DEBUG) {Serial.println("main5 - check watering time");}
  i = checkWateringTime(last_warering_time);
  if ( i > 0 ) {
    last_warering_time = ((watering_time[i - 1]/100) * 60) + (watering_time[i - 1] - ((watering_time[i - 1]/100)*100));
    watering_flg = 1;
  }
  if(DEBUG) {Serial.print("main6 - watering_flg: ");}
  if(DEBUG) {Serial.println(watering_flg);}
  // Check water level
  if ( watering_flg == 1 ) {
    water_level = getWaterLevel();
    sprintf(timestamp_str, "%02d.%02d.20%02d %02d:%02d:%02d",dayOfMonth,month,year,hour,minute,second);
    sprintf(water_level_str, "%d", water_level);
    esp_str = "L|" + (String)timestamp_str + '|' + water_level_str + '|';
    ESP.println(esp_str);
    delay(10);

    readEspCommand();
    
    sprintf(log_str, "WATER LEVEL: %d", water_level);
    if(DEBUG) {Serial.println(log_str);}
    if ( water_level < min_water_level ) {
      // Emergency pump STOP - Water LOW lewel
      if(DEBUG) {Serial.println("main - Emergency pump STOP - Water LOW lewel");}
      i = 0;
      while ( i <= 2 ) {
        digitalWrite(pump_control_pin[i], LOW);
        pump_watering_duration[i] = 0;
        i++;
      }
      pump_status = 0;
      watering_flg = 0;
      watering_in_progress = 0;
    }
  }
  if(DEBUG) {Serial.print("main7 - watering progress flag: ");}
  if(DEBUG) {Serial.println(watering_in_progress);}
  
  // Watering - calculate watering time
  if ( watering_flg == 1 ) {
    esp_wtr_all_str = "";
    watering_in_progress = 0;
    i = 0;
    while( i <= 2 ){
      if(DEBUG) {Serial.print("PUMP[");}
      if(DEBUG) {Serial.print(i);}
      if(DEBUG) {Serial.println("]:");}
      if ( (hum[i] > max_hum) || (hum[i] < 5)  ) {
        // Fully watered or sensor disconnected
        watering_duration[i] = 0; // No watering need
        watering_in_progress = watering_in_progress | 0;
        last_pump_millis[i] = millis();
        if(DEBUG) {Serial.println("main8 - calc watering time - No watering needed");}
      }
      else if ( hum[i] >= half_hum && hum[i] < max_hum ) {
        watering_duration[i] = max_watering_duration / 2;
        watering_in_progress = watering_in_progress | 1;
        last_pump_millis[i] = millis();  // Set start of actual watering
        if(DEBUG) {Serial.println("main8 - calc watering time - Half watering");}
      }
      else if ( hum[i] < half_hum ) {
        watering_duration[i] = max_watering_duration;
        watering_in_progress = watering_in_progress | 1;
        last_pump_millis[i] = millis();  // Set start of actual watering
        if(DEBUG) {Serial.println("main8 - calc watering time - Full watering");}
      }
      sprintf(watering_duration_str, "%d", watering_duration[i]);
      esp_wtr_all_str = esp_wtr_all_str + '|' + String(watering_duration_str);
      sprintf(log_str, "WATERING #%1d: %d", i, watering_duration[i]);
      if(DEBUG) {Serial.println(log_str);}
      i++;
    }
    //Serial.println(esp_wtr_all_str);
    sprintf(timestamp_str, "%02d.%02d.20%02d %02d:%02d:%02d",dayOfMonth,month,year,hour,minute,second);
    esp_str = "W|" + (String)timestamp_str + esp_wtr_all_str + '|';
    //ESP.println(esp_str);
    if(DEBUG) {Serial.print("main9 - Watering message: ");}
    if(DEBUG) {Serial.println(esp_str);}
    if(DEBUG) {Serial.print("main10 - watering_in_progress: ");}
    if(DEBUG) {Serial.println(watering_in_progress);}
    delay(10);

    readEspCommand();

    watering_flg = 0;
  }
  // Watering - pumps control
  water_level = getWaterLevel();
  if ( water_level < min_water_level ) {
    // Emergency pump STOP - Water LOW lewel
    if(DEBUG) {Serial.println("main - Emergency pump STOP - Water LOW lewel");}
    i = 0;
    while ( i <= 2 ) {
      digitalWrite(pump_control_pin[i], LOW);
      pump_watering_duration[i] = 0;
      i++;
    }
    pump_status = 0;
    watering_flg = 0;
    watering_in_progress = 0;
  }
  
  if ( (watering_in_progress > 0) && (water_level > min_water_level) ) {
    act_millis = millis();
    i = 0;
    // Calculation PUMPs duration
    while ( i <= 2 ) {
      if (watering_duration[i] > 0) {
        if ( act_millis >= last_pump_millis[i] ) {
          pump_watering_duration[i] = pump_watering_duration[i] + (act_millis - last_pump_millis[i]);
        } else {
          pump_watering_duration[i] = pump_watering_duration[i] + act_millis;
        }
        last_pump_millis[i] = act_millis;
      } else {
        pump_watering_duration[i] = 0l;
      }
      if(DEBUG) {Serial.print("main - pump duration [");}
      if(DEBUG) {Serial.print(i);}
      if(DEBUG) {Serial.print("]: ");}
      if(DEBUG) {Serial.println(pump_watering_duration[i]);}
      i++;
    }
    sprintf(log_str, "D:Real Watering Time: %d | %d | %d",pump_watering_duration[0],pump_watering_duration[1],pump_watering_duration[2]);
    if(DEBUG) {Serial.println((String)log_str);}
    //ESP.println(log_str);
    //delay(10);
    
    // START / STOP Pumps
    i = 0;
    while ( i <= 2 ) {
      if ( (pump_watering_duration[i] < (long)(watering_duration[i] * 1000l)) && (watering_duration[i] > 0l)) {
        digitalWrite(pump_control_pin[i], HIGH);
        pump_status = pump_status | (1 << i);
        sprintf(log_str, "PUMP:%d ON - pump_status:%d",i,pump_status);
        if(DEBUG) {Serial.println((String)log_str);}
      } else {
        digitalWrite(pump_control_pin[i], LOW);
        pump_status = pump_status & (0xFF - (1 << i));
        sprintf(log_str, "PUMP:%d OFF - pump_status:%d",i,pump_status);
        if(DEBUG) {Serial.println((String)log_str);}
      }
      i++;
    }

    if ( pump_status == 0 ) {
      watering_in_progress = 0;
      esp_wtr_all_str = "";
      watering_flg = 0;
      i = 0;
      while( i <= 2 ){
        sprintf(watering_duration_str, "%d", (pump_watering_duration[i] / 1000));
        esp_wtr_all_str = esp_wtr_all_str + '|' + String(watering_duration_str);
        pump_watering_duration[i] = 0l;
        i++;
      }
      sprintf(timestamp_str, "%02d.%02d.20%02d %02d:%02d:%02d",dayOfMonth,month,year,hour,minute,second);
      esp_str = "W|" + (String)timestamp_str + esp_wtr_all_str + '|';
      ESP.println(esp_str);
      delay(20);
    }
  }
  
  if(DEBUG) {Serial.println(String(millis()));}
  // Read Humidity - read_hum_interval
  if ( millis() >= (lastMillisHum + ((long)read_hum_interval * 1000l)) ) {
    lastMillisHum = millis();

    i = 0;
    while( i <= 2 ){
      hum[i] = readHumidity(i);
      delay(100);
      i++;
    }

    esp_str = "";
    esp_hum_all_str = "";
    
    sprintf(timestamp_str, "%02d.%02d.20%02d %02d:%02d:%02d",dayOfMonth,month,year,hour,minute,second);

    if(DEBUG) {Serial.println(timestamp_str);}
    i = 0;
    while( i <= 2 ){
      dtostrf(hum[i], 3, 2, hum_str);
      esp_hum_all_str = esp_hum_all_str + '|' + hum_str;
      sprintf(log_str, "Sensor #%d: %s", i, hum_str);
      if(DEBUG) {Serial.println(log_str);}
      i++;
    }
    
    esp_str = "M|" + (String)timestamp_str + esp_hum_all_str + '|';
    
    ESP.println(esp_str);
    delay(100);

    water_level = getWaterLevel();
    sprintf(timestamp_str, "%02d.%02d.20%02d %02d:%02d:%02d",dayOfMonth,month,year,hour,minute,second);
    sprintf(water_level_str, "%d", water_level);
    esp_str = "L|" + (String)timestamp_str + '|' + water_level_str + '|';
    ESP.println(esp_str);
    delay(10);


    readEspCommand();
    
  }

  readEspCommand();
  
  if(DEBUG) {Serial.println("main - end");}
  
  delay(10); // dame si chvili pauzu
}

