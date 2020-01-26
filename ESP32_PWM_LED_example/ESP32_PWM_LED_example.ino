int freq = 5000;
int ledChannel = 0;
int resolution = 8;
int ledPin = 5;
 
void setup() {
  Serial.begin(115200);
 
  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(ledPin, ledChannel);

  Serial.print("Setup ... END");
}
 
void loop() {
  Serial.print("Loop ... ");
  for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle++){
     ledcWrite(ledChannel, dutyCycle);
     delay(7);
  }
 
  for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle--){
     ledcWrite(ledChannel, dutyCycle);
     delay(7);
  }
 
}

