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
  distanceToTarget = 6371000 * c; // Earth's radius in meters
}