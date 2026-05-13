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