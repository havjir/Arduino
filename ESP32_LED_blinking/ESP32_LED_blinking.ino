// ESP23 moduke ESPESSIF ESP32-WROOM-32 (LED_PIN=2)
int ledPin = 2;

void setup()
{
    pinMode(ledPin, OUTPUT);
    Serial.begin(115200);
}

void loop()
{
    Serial.println("Hello, world!");
    digitalWrite(ledPin, HIGH);
    delay(5000);
    digitalWrite(ledPin, LOW);
    delay(5000);
}
