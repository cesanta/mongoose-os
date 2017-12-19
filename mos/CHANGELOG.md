Mos tool changelog
==================

## Latest (pending release)

- Fix brew build: mos shouldn't check for updates because when installed via
  brew it should be updated also via brew

## 1.22.1

Mos tool:

- Add support for brew

## 1.22

Mos tool:

- Fix aws thing name for the cases when it's not the default `<arch>_<mac>`

Firmware:

- BT: Add API for managing pairing; improve security
- ESP32: update ESP-IDF
- ota-aws-shadow library is improved and renamed to ota-shadow
- Add events API, see [mgos_event.h](https://mongoose-os.com/docs/api/mgos_event.h.html)
