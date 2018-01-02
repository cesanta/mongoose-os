Mos tool changelog
==================

## Latest (pending release)

Nothing yet

## 1.23

Mos tool:

- Fix brew build: mos shouldn't check for updates because when installed via
  brew it should be updated also via brew

Firmware:

- Implemented `mgos_event_add_group_handler()` in C and
  `Event.addGroupHandler()` in mJS
- `mgos_net_add_event_handler()` is deprecated in favor of `mgos_event`-based
  API. See comment for `MGOS_EVENT_GRP_NET` for the example.
- Increased CC32xx UART buffer size
- Many improvements in dashboard support (and in the dashboard itself)
- Added DigiCert root

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
