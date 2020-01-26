#define echoPin 7 // Echo Pin
#define trigPin 8 // Trigger Pin
#define LEDPin 13 // Onboard LED

int maximumRange = 200; // Maximum range needed
int minimumRange = 0; // Minimum range needed
//long duration, distance; // Duration used to calculate distance
long xxx, last_xxx;

// Buzzer setup
int speakerPin = 9; //Buzzer pin
int length = 15; // the number of notes
char notes[] = "ceg cegeedefd "; // a space represents a rest
int beats[] = { 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 4 };
int tempo = 50;

void playTone(int tone, int duration) {
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    digitalWrite(speakerPin, HIGH);
    delayMicroseconds(tone);
    digitalWrite(speakerPin, LOW);
    delayMicroseconds(tone);
  }
}

void playNote(char note, int duration) {
  char names[] = { 'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C' };
  int tones[] = { 1915, 1700, 1519, 1432, 1275, 1136, 1014, 956 };

  // play the tone corresponding to the note name
  for (int i = 0; i < 8; i++) {
    if (names[i] == note) {
      playTone(tones[i], duration);
    }
  }
}

long mesure() {
  long duration, distance; // Duration used to calculate distance
  /* The following trigPin/echoPin cycle is used to determine the
 distance of the nearest object by bouncing soundwaves off of it. */ 
 digitalWrite(trigPin, LOW); 
 delayMicroseconds(2); 

 digitalWrite(trigPin, HIGH);
 delayMicroseconds(10); 
 
 digitalWrite(trigPin, LOW);
 duration = pulseIn(echoPin, HIGH);
 
 //Calculate the distance (in cm) based on the speed of sound.
 distance = duration/58.2;
 
 if (distance >= maximumRange || distance <= minimumRange){
 /* Send a negative number to computer and Turn LED ON 
 to indicate "out of range" */
 return -1;

 }
 else {
 /* Send the distance to the computer using Serial protocol, and
 turn LED OFF to indicate successful reading. */
 return distance;

 }
  
}

void setup() {
 // Sensor
 Serial.begin (9600);
 pinMode(trigPin, OUTPUT);
 pinMode(echoPin, INPUT);
 last_xxx = mesure();

 // Buzzer
 pinMode(speakerPin, OUTPUT);

 
}


void loop() {

    xxx = mesure();

    if ( abs(xxx - last_xxx) > 10 ){
      
    
      for (int j = 0; j < 1; j++) {
        for (int i = 0; i < length; i++) {
          if (notes[i] == ' ') {
            delay(beats[i] * tempo); // rest
          } else {
            playNote(notes[i], beats[i] * tempo);
          }
          // pause between notes
          delay(tempo / 2); 
        }
      }

    } else {
      delay(200);
    }

    last_xxx = xxx;
 

 
}
