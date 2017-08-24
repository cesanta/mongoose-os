/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

package cc32xx

import (
	"cesanta.com/mos/flash/common"
)

const (
	servicePackImgName = "/sys/servicepack.ucf"
	bootImgName        = "/sys/mcuimg.bin"
	BootFlashImgName   = "/sys/mcuflashimg.bin"

	PartTypeServicePack      = "service_pack"
	PartTypeCABundle         = "cabundle"
	PartTypeCertificate      = "cert"
	PartTypeSLFile           = "slfile"
	PartTypeSLConfig         = "slcfg"
	PartTypeBootLoader       = "boot"
	PartTypeBootLoaderConfig = "boot_cfg"
	PartTypeApp              = "app"
	PartTypeFSContainer      = "fs"
	PartTypeSignature        = "sig"
)

type PartsByTypeAndName []*common.FirmwarePart

func (pp PartsByTypeAndName) Len() int      { return len(pp) }
func (pp PartsByTypeAndName) Swap(i, j int) { pp[i], pp[j] = pp[j], pp[i] }
func (pp PartsByTypeAndName) Less(i, j int) bool {
	pi, pj := pp[i], pp[j]
	// 1. Service pack (there's only one).
	piIsServicePack := (pi.Type == PartTypeServicePack || pi.Name == servicePackImgName)
	pjIsServicePack := (pj.Type == PartTypeServicePack || pj.Name == servicePackImgName)
	if piIsServicePack || pjIsServicePack {
		return piIsServicePack && !pjIsServicePack
	}
	// 2. CA bundle
	// Note: During image extraction, signed files (e.g. app image) are verified,
	// so CA bundle and cert(s) must have already been written.
	if pi.Type == PartTypeCABundle || pj.Type == PartTypeCABundle {
		return pi.Type == PartTypeCABundle && pj.Type != PartTypeCABundle
	}
	// 3. Certificates.
	if pi.Type == PartTypeCertificate || pj.Type == PartTypeCertificate {
		if pi.Type == PartTypeCertificate && pj.Type != PartTypeCertificate {
			return true
		}
		if pi.Type == PartTypeCertificate && pj.Type == PartTypeCertificate {
			return pi.Name < pj.Name
		}
		return false
	}
	// 4. Boot loader image (there's only one).
	piIsBoot := (pi.Type == PartTypeBootLoader || pi.Name == bootImgName)
	pjIsBoot := (pj.Type == PartTypeBootLoader || pj.Name == bootImgName)
	if piIsBoot || pjIsBoot {
		return piIsBoot && !pjIsBoot
	}
	// 5. Boot loader configs.
	if pi.Type == PartTypeBootLoaderConfig || pj.Type == PartTypeBootLoaderConfig {
		if pi.Type == PartTypeBootLoaderConfig && pj.Type != PartTypeBootLoaderConfig {
			return true
		}
		if pi.Type == PartTypeBootLoaderConfig && pj.Type == PartTypeBootLoaderConfig {
			return pi.Name < pj.Name
		}
		return false
	}
	// 6. App code image.
	if pi.Type == PartTypeApp || pj.Type == PartTypeApp {
		if pi.Type == PartTypeApp && pj.Type != PartTypeApp {
			return true
		}
		if pi.Type == PartTypeApp && pj.Type == PartTypeApp {
			return pi.Name < pj.Name
		}
		return false
	}
	// 7. FS containers.
	if pi.Type == PartTypeFSContainer || pj.Type == PartTypeFSContainer {
		if pi.Type == PartTypeFSContainer && pj.Type != PartTypeFSContainer {
			return true
		}
		if pi.Type == PartTypeFSContainer && pj.Type == PartTypeFSContainer {
			return pi.Name < pj.Name
		}
		return false
	}
	// 8. SimpleLink configs.
	if pi.Type == PartTypeSLConfig || pj.Type == PartTypeSLConfig {
		if pi.Type == PartTypeSLConfig && pj.Type != PartTypeSLConfig {
			return true
		}
		if pi.Type == PartTypeSLConfig && pj.Type == PartTypeSLConfig {
			return pi.Name < pj.Name
		}
		return false
	}
	// 9. The rest, sorted by name.
	return pi.Name < pj.Name
}
