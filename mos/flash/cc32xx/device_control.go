package cc32xx

type DeviceControl interface {
	EnterBootLoader() error
	BootFirmware() error
	Close()
}
