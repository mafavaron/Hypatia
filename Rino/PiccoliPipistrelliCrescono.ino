// Rino V1.0
//
// Just a demonstrator for the concept

const unsigned int TRIG_PIN  = 13;
const unsigned int ECHO_PIN  = 12;
const unsigned int BAUD_RATE = 9600;

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  Serial.begin(BAUD_RATE);
}

void askReading() {
  // Trigger sequence for the HY-SRF05: asks for a measurement
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
}


float getReading() {

  float distance;
  
  const unsigned long duration = pulseIn(ECHO_PIN, HIGH);
  if(duration <= 0) {
    distance = -9999.9f;
  }
  else {
    distance = duration / 58.0f;
  }

  return(distance);
  
}


void loop() {

  askReading();
  float distance = getReading();
  Serial.print("Distance: ");
  Serial.println(distance);
  
  delay(500);
  
}
