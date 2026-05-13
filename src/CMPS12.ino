#define CMPS_GET_ANGLE16 0x13

void setupCMPS12() {
  Serial3.begin(9600);
  Serial.println("CMPS12 compass started");
}

int getHeading() {
  unsigned char high_byte, low_byte;
  unsigned int angle16;

  Serial3.write(CMPS_GET_ANGLE16);
  while (Serial3.available() < 2);
  high_byte = Serial3.read();
  low_byte = Serial3.read();

  angle16 = (high_byte << 8) + low_byte;
  return angle16 / 10;  // degrees 0-359.9 scaled by 10
}
