#include <stdbool.h>

#include "mgos.h"
#include "mgos_app.h"

#include "mgos_location.h"

static bool is_lat_valid(double lat) {
  return lat >= -90.0 && lat <= 90.0;
}

static bool is_lon_valid(double lon) {
  return lon >= -180.0 && lon <= 180.0;
}

bool mgos_location_get(struct mgos_location_lat_lon *loc) {
  loc->lat = mgos_sys_config_get_device_location_lat();
  loc->lon = mgos_sys_config_get_device_location_lon();

  if (!is_lat_valid(loc->lat) || !is_lon_valid(loc->lon)) {
    return false;
  }

  return true;
}

bool mgos_location_init(void) {
  return true;
}
