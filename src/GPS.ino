Serial2.begin(9600);  // Initializes serial port for GPS

void readGPS() {
  while (Serial2.available())
    gps.encode(Serial2.read());
}

if (gps.location.isValid()) {
  boatLat = gps.location.lat();
  boatLng = gps.location.lng();
  lastLat = boatLat;
  lastLng = boatLng;
} else {
  boatLat = lastLat;
  boatLng = lastLng;
}
