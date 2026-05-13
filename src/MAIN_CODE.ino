#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <TinyGPS++.h>
#include <SD.h>

// ======================= RC Backup Setup =======================
#define RC_RUDDER_PIN 2  // Rudder channel (CH1)
#define RC_SAIL_PIN 3    // Sail channel (CH3)

// Servo pulse width limits
#define SERVOMIN_SAIL   230
#define SERVOMAX_SAIL   420
#define SERVOMIN_RUDDER 150
#define SERVOMAX_RUDDER 600

// PWM driver channels
#define SERVO_RUDDER_CHANNEL 1
#define SERVO_SAIL_CHANNEL 0

// ======================= Autonomous Setup =======================
#define CMPS_GET_ANGLE16 0x13
#define WindSensorPin 20
#define WindDirectionPin A14
#define SD_CS 53

TinyGPSPlus gps;
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
File dataFile;

const double targetLat = 52.483595 ;  // Point B
const double targetLng = -1.881704; // Point B

double boatLat = 0, boatLng = 0, lastLat = 0, lastLng = 0;
int heading = 0, bearingToTarget = 0;
double distanceToTarget = 0;
int rudderAngle = 90, sailAngle = 90;
int windDirection = 0, windRelative = 0;
bool missionComplete = false;

volatile int rc_rudder_pulse = 0, rc_sail_pulse = 0;
volatile unsigned long rc_rudder_start = 0, rc_sail_start = 0;

volatile unsigned long rotations = 0, contactBounceTime = 0;
unsigned long lastMeasureTime = 0;
float windSpeed = 0;
int vaneValue = 0, calibratedDirection = 0;
#define Offset 0

// ======================= RC ISRs =======================
void rudder_isr() {
  if (digitalRead(RC_RUDDER_PIN) == HIGH)
    rc_rudder_start = micros();
  else
    rc_rudder_pulse = micros() - rc_rudder_start;
}

void sail_isr() {
  if (digitalRead(RC_SAIL_PIN) == HIGH)
    rc_sail_start = micros();
  else
    rc_sail_pulse = micros() - rc_sail_start;
}

void isr_rotation() {
  if ((millis() - contactBounceTime) > 15) {
    rotations++;
    contactBounceTime = millis();
  }
}

// ======================= Setup =======================
void setup() {
  Serial.begin(115200);
  Wire.begin();

  Serial2.begin(9600);
  Serial3.begin(9600);

  pinMode(WindSensorPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(WindSensorPin), isr_rotation, FALLING);
  lastMeasureTime = millis();

  pinMode(RC_RUDDER_PIN, INPUT_PULLUP);
  pinMode(RC_SAIL_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RC_RUDDER_PIN), rudder_isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(RC_SAIL_PIN), sail_isr, CHANGE);

  pwm.begin();
  pwm.setPWMFreq(50);
  delay(10);

  if (!SD.begin(SD_CS)) {
    Serial.println("❌ SD card initialization failed!");
  } else {
    Serial.println("✅ SD card ready");
  }

  Serial.println("✅ Boat ready. Default: Autonomous from A to B. RC backup ready.");
}

// ======================= Main Loop =======================
void loop() {
  int rudder_pulse, sail_pulse;
  noInterrupts();
  rudder_pulse = rc_rudder_pulse;
  sail_pulse = rc_sail_pulse;
  interrupts();

  if (rudder_pulse > 900 && rudder_pulse < 2200 && sail_pulse > 900 && sail_pulse < 2200) {
    // RC signal detected -> Manual override
    runManualMode(rudder_pulse, sail_pulse);
  } else {
    // No RC -> Autonomous mode
    runAutonomousMode();
  }

  delay(50);
}

// ======================= Manual Mode =======================
void runManualMode(int rudder_pulse, int sail_pulse) {
  Serial.println("--- MANUAL MODE ---");

  int rudderAngleRC = map(rudder_pulse, 1000, 2000, 0, 180);
  int sailAngleRC = map(sail_pulse, 1000, 2000, 20, 90);

  rudderAngleRC = constrain(rudderAngleRC, 0, 180);
  sailAngleRC = constrain(sailAngleRC, 20, 90);

  setServoAngle(SERVO_RUDDER_CHANNEL, rudderAngleRC);
  setServoAngle(SERVO_SAIL_CHANNEL, sailAngleRC);

  Serial.print("RC Rudder: "); Serial.print(rudderAngleRC);
  Serial.print(" | RC Sail: "); Serial.println(sailAngleRC);
}

// ======================= Autonomous Mode =======================
void runAutonomousMode() {
  Serial.println("--- AUTONOMOUS MODE ---");

  readGPS();
  heading = getHeading();
  readWind();

  if (gps.location.isValid()) {
    boatLat = gps.location.lat();
    boatLng = gps.location.lng();
    lastLat = boatLat;
    lastLng = boatLng;
  } else {
    boatLat = lastLat;
    boatLng = lastLng;
  }

  if (!missionComplete) {
    calculateBearingAndDistanceTo(targetLat, targetLng);
    if (distanceToTarget < 3.0) {
      missionComplete = true;
      Serial.println("✅ Reached Point B. Mission complete.");
      rudderAngle = 90;
      sailAngle = 90;
      setServoAngle(SERVO_RUDDER_CHANNEL, rudderAngle);
      setServoAngle(SERVO_SAIL_CHANNEL, sailAngle);
      logToSD();
      return;
    }
  }

  windRelative = calibratedDirection - heading;
  if (windRelative < 0) windRelative += 360;

  rudderAngle = controlRudder(heading, bearingToTarget);
  sailAngle = controlSail(windRelative);

  setServoAngle(SERVO_RUDDER_CHANNEL, rudderAngle);
  setServoAngle(SERVO_SAIL_CHANNEL, sailAngle);

  printDebug();
  logToSD();
}

// ======================= Support Functions =======================
void readGPS() {
  while (Serial2.available())
    gps.encode(Serial2.read());
}

int getHeading() {
  Serial3.write(CMPS_GET_ANGLE16);
  while (Serial3.available() < 2);
  unsigned char high_byte = Serial3.read();
  unsigned char low_byte = Serial3.read();
  unsigned int angle16 = (high_byte << 8) + low_byte;
  return angle16 / 10;
}

void readWind() {
  unsigned long currentTime = millis();
  if (currentTime - lastMeasureTime >= 1000) {
    noInterrupts();
    unsigned long rotCopy = rotations;
    rotations = 0;
    interrupts();

    windSpeed = rotCopy * 0.75;

    vaneValue = analogRead(WindDirectionPin);
    windDirection = map(vaneValue, 0, 1023, 0, 360);
    calibratedDirection = windDirection + Offset;
    if (calibratedDirection >= 360) calibratedDirection -= 360;
    if (calibratedDirection < 0) calibratedDirection += 360;

    lastMeasureTime = currentTime;
  }
}

void calculateBearingAndDistanceTo(double targetLat, double targetLng) {
  double lat1 = radians(boatLat);
  double lon1 = radians(boatLng);
  double lat2 = radians(targetLat);
  double lon2 = radians(targetLng);

  double dLon = lon2 - lon1;
  double y = sin(dLon) * cos(lat2);
  double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLon);
  double brng = atan2(y, x);
  brng = degrees(brng);
  if (brng < 0) brng += 360;
  bearingToTarget = (int)brng;

  double dLat = lat2 - lat1;
  double a = pow(sin(dLat / 2), 2) + cos(lat1) * cos(lat2) * pow(sin(dLon / 2), 2);
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));
  distanceToTarget = 6371000 * c;
}

int controlRudder(int currentHeading, int targetBearing) {
  int error = targetBearing - currentHeading;
  if (error > 180) error -= 360;
  if (error < -180) error += 360;
  int angle = constrain(error * 1.5, -45, 45);
  return angle + 90;
}

int controlSail(int windRel) {
  if (windRel > 180) windRel = 360 - windRel;
  int angle = map(windRel, 0, 90, 20, 90);
  return constrain(angle, 20, 90);
}

void setServoAngle(uint8_t channel, int angle) {
  int pulse = map(angle, 0, 180, 102, 512);
  pwm.setPWM(channel, 0, pulse);
}

void printDebug() {
  Serial.println("------------------------------");
  Serial.print("Lat: "); Serial.print(boatLat, 6);
  Serial.print(" | Lng: "); Serial.println(boatLng, 6);
  Serial.print("Heading: "); Serial.print(heading);
  Serial.print(" | Bearing: "); Serial.println(bearingToTarget);
  Serial.print("Distance: "); Serial.print(distanceToTarget);
  Serial.print(" | Wind Dir: "); Serial.println(calibratedDirection);
  Serial.print("Wind Rel: "); Serial.print(windRelative);
  Serial.print(" | Rudder: "); Serial.print(rudderAngle);
  Serial.print(" | Sail: "); Serial.println(sailAngle);
  Serial.print("Time: "); Serial.println(millis());
}

void logToSD() {
  dataFile = SD.open("log.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.println("================================");
    dataFile.println("Date: 09/07/2025");  // <-- Manually typed date
    dataFile.print("Time (ms): "); dataFile.println(millis());
    dataFile.print("Lat: "); dataFile.println(boatLat, 6);
    dataFile.print("Lng: "); dataFile.println(boatLng, 6);
    dataFile.print("Heading: "); dataFile.println(heading);
    dataFile.print("Bearing: "); dataFile.println(bearingToTarget);
    dataFile.print("Distance: "); dataFile.println(distanceToTarget);
    dataFile.print("Wind Dir: "); dataFile.println(calibratedDirection);
    dataFile.print("Wind Rel: "); dataFile.println(windRelative);
    dataFile.print("Rudder: "); dataFile.println(rudderAngle);
    dataFile.print("Sail: "); dataFile.println(sailAngle);
    dataFile.println("================================");
    dataFile.close();
  } else {
    Serial.println("⚠️ Error writing to SD card");
  }
}