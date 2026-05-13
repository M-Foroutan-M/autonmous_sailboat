#define WindSensorPin 18
#define WindDirectionPin A14

volatile unsigned long rotations = 0;
volatile unsigned long contactBounceTime = 0;

float windSpeed = 0;

int VaneValue = 0;
int direction = 0;
int calibratedDirection = 0;
int lastDirection = 0;

#define Offset 0

unsigned long lastMeasureTime = 0;

void setupWind() {
  pinMode(WindSensorPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(WindSensorPin), isr_rotation, FALLING);
  lastMeasureTime = millis();
  Serial.println("Wind sensor started");
}

void readWind() {
  unsigned long currentTime = millis();
  if (currentTime - lastMeasureTime >= 1000) {
    noInterrupts();
    unsigned long rotCopy = rotations;
    rotations = 0;
    interrupts();

    windSpeed = rotCopy * 0.75;  // adjust for your anemometer

    VaneValue = analogRead(WindDirectionPin);
    direction = map(VaneValue, 0, 1023, 0, 360);
    calibratedDirection = direction + Offset;
    if (calibratedDirection > 360) calibratedDirection -= 360;
    if (calibratedDirection < 0) calibratedDirection += 360;

    lastDirection = calibratedDirection;
    lastMeasureTime = currentTime;
  }
}

int getWindDirectionCalibrated() {
  return calibratedDirection;
}

int getWindRelative() {
  // Relative wind calculation happens in main using heading and calibratedDirection
  return 0;
}

void isr_rotation() {
  if ((millis() - contactBounceTime) > 15) {
    rotations++;
    contactBounceTime = millis();
  }
}
