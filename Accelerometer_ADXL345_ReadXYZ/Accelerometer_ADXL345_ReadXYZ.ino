#include <Wire.h>
#include <ADXL345.h>

#define CAL_SAMPLES 100

ADXL345 accel(ADXL345_ALT);

/// Calibration variables
float f_cal_X = 0;
float f_cal_Y = 0;
float f_cal_Z = 0;
int   i_cal_cnt = 200;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  byte deviceID = accel.readDeviceID();
  if (deviceID != 0) {
    Serial.print("0x");
    Serial.print(deviceID, HEX);
    Serial.println("");
  } else {
    Serial.println("read device id: failed");
    while(1) {
      delay(100);
    }
  }

  // Data Rate
  // - ADXL345_RATE_3200HZ: 3200 Hz
  // - ADXL345_RATE_1600HZ: 1600 Hz
  // - ADXL345_RATE_800HZ:  800 Hz
  // - ADXL345_RATE_400HZ:  400 Hz
  // - ADXL345_RATE_200HZ:  200 Hz
  // - ADXL345_RATE_100HZ:  100 Hz
  // - ADXL345_RATE_50HZ:   50 Hz
  // - ADXL345_RATE_25HZ:   25 Hz
  // - ...
  if (!accel.writeRate(ADXL345_RATE_200HZ)) {
    Serial.println("write rate: failed");
    while(1) {
      delay(100);
    }
  }

  // Data Range
  // - ADXL345_RANGE_2G: +-2 g
  // - ADXL345_RANGE_4G: +-4 g
  // - ADXL345_RANGE_8G: +-8 g
  // - ADXL345_RANGE_16G: +-16 g
  if (!accel.writeRange(ADXL345_RANGE_16G)) {
    Serial.println("write range: failed");
    while(1) {
      delay(100);
    }
  }

  if (!accel.start()) {
    Serial.println("start: failed");
    while(1) {
      delay(100);
    }
  }

  delay(500);
  calibrate(CAL_SAMPLES);
}

void calibrate(uint8_t samples) {
  Serial.println("Start - Calibration");
  int i = 0;
  while (accel.update() && i < samples ) {
    f_cal_X += accel.getX();
    f_cal_Y += accel.getY();
    f_cal_Z += accel.getZ();
    i++;
    delay(50);
  }
  Serial.print("XXXXXXXXXXXXXXXXXXXXXX ");
  Serial.print(f_cal_X);
  Serial.print(" ; ");
  Serial.print(f_cal_Y);
  Serial.print(" ; ");
  Serial.print(f_cal_Z);
  Serial.println("");
  f_cal_X = f_cal_X / samples;
  f_cal_Y = f_cal_Y / samples;
  f_cal_Z = f_cal_Z / samples;
  Serial.print("End Calibration: ");
  Serial.print(f_cal_X);
  Serial.print(" ; ");
  Serial.print(f_cal_Y);
  Serial.print(" ; ");
  Serial.print(f_cal_Z);
  Serial.println("");
}

void loop() {
  if (accel.update()) {
    Serial.print((accel.getX() - f_cal_X));
    Serial.print(",");
    Serial.print((accel.getY() - f_cal_Y));
    Serial.print(",");
    Serial.print((accel.getZ() - f_cal_Z));
    Serial.println("");
  } else {
    Serial.println("update failed");
    //while(1) {
      delay(100);
    //}
  }
  delay(30);
}
