// Rino V1.0
//
// Just a demonstrator for the concept

const unsigned int TRIG_PIN  = 13;
const unsigned int ECHO_PIN  = 12;
const unsigned int BAUD_RATE = 9600;

const int numPingsPerReading = 10;       // measurement repetitions in any reading
const long int timeStepBetweenReadings = 10; // milliseconds


float sonarPing() {

  // Trigger sequence for the HY-SRF05: asks for a measurement
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Harvest the measurement and yield it back
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


int readDistance(float* distance, float* stdDev, const int numPings = 5, const long int timeStep = 10L) {

  // Assume success (will falsify on failure)
  int iRetCode = 0;

  // Send a sequence of sonar pings, get individual distances from them,
  // and compute their statistics
  *distance = 0.0f;
  float squaredDistance = 0.0f;
  *stdDev   = 0.0f;
  for(int iPing=0; iPing < numPings; iPing++) {
    float dst = sonarPing();
    *distance += dst;
    squaredDistance += dst*dst;
    delay(timeStep);
  }
  *distance /= numPings;
  *stdDev   =  sqrt(squaredDistance/numPings - ((*distance) * (*distance)));

  return(iRetCode);
  
}


void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(BAUD_RATE);
}


void loop() {

  float distance;
  float stdDev;
  digitalWrite(LED_BUILTIN, HIGH);
  int iRetCode = readDistance(&distance, &stdDev, numPingsPerReading, timeStepBetweenReadings);
  digitalWrite(LED_BUILTIN, LOW);
  if((iRetCode == 0) & (stdDev < 1.0f)) {
    if(distance < 150.0f) {
      Serial.print("La mia preda è a ");
      Serial.print(distance);
      Serial.println(" centimetri.");
    }
    else {
      Serial.println("Vicino non sento niente!");
    }
  }
  else {
      Serial.println("Vicino non sento niente!");
  }
  if(distance < 150.f) {
    delay(1000 - (int)(6.0f*(150.f - distance)));
  }
  else {
    delay(1000);
  }
  
}

