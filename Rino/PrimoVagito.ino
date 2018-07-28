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

void loop() {

  // Trigger sequence for the HY-SRF05: asks for a measurement
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Actual data read
  const unsigned long duration = pulseIn(ECHO_PIN, HIGH);
  if(duration <= 0) {
    Serial.println("No pulse duration...");
  }
  else {
    int distance = duration / 29 / 2;
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
  }

  delay(100);
  
}
