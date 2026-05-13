void logToSD() {
  dataFile = SD.open("log.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.println("================================");
    dataFile.print("Time (ms): "); dataFile.println(millis());
    dataFile.print("Lat: "); dataFile.println(boatLat, 6);
    dataFile.print("Lng: "); dataFile.println(boatLng, 6);
    dataFile.print("Heading: "); dataFile.println(heading);
    dataFile.print("BearingToTarget: "); dataFile.println(bearingToTarget);
    dataFile.print("DistanceToTarget: "); dataFile.println(distanceToTarget);
    dataFile.print("WindDirection: "); dataFile.println(windDirection);
    dataFile.print("CalibratedDirection: "); dataFile.println(calibratedDirection);
    dataFile.print("WindRelative: "); dataFile.println(windRelative);
    dataFile.print("Rudder Angle: "); dataFile.println(rudderAngle);
    dataFile.print("Sail Angle: "); dataFile.println(sailAngle);
    dataFile.println("================================");
    dataFile.close();
  }
}
