#include <stdbool.h>
#include "mgos_location.h"

static struct mgos_location_lat_lon s_loc = {0.0, 0.0};

static bool is_lat_valid(double lat) {
  return lat >= -90.0 && lat <= 90.0;
}

static bool is_lon_valid(double lon) {
  return lon >= -180.0 && lon <= 180.0;
}

bool mgos_location_get(struct mgos_location_lat_lon *loc) {
  loc->lat = s_loc.lat;
  loc->lon = s_loc.lon;

  if (!is_lat_valid(loc->lat) || !is_lon_valid(loc->lon)) {
    return false;
  }

  return true;
}

void location_set(double lat, double lon) {
  s_loc.lat = lat;
  s_loc.lon = lon;
}
