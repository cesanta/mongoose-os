package cc3200

type DeviceControl interface {
	EnterBootLoader() error
	BootFirmware() error
}
