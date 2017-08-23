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
	PartTypeSLFile           = "slfile"
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
	// Service pack goes first (there's only one).
	piIsServicePack := (pi.Type == PartTypeServicePack || pi.Name == servicePackImgName)
	pjIsServicePack := (pj.Type == PartTypeServicePack || pj.Name == servicePackImgName)
	if piIsServicePack || pjIsServicePack {
		return piIsServicePack && !pjIsServicePack
	}
	// Then boot image (there's only one).
	piIsBoot := (pi.Type == PartTypeBootLoader || pi.Name == bootImgName)
	pjIsBoot := (pj.Type == PartTypeBootLoader || pj.Name == bootImgName)
	if piIsBoot || pjIsBoot {
		return piIsBoot && !pjIsBoot
	}
	// Then boot configs.
	if pi.Type == PartTypeBootLoaderConfig || pj.Type == PartTypeBootLoaderConfig {
		if pi.Type == PartTypeBootLoaderConfig && pj.Type != PartTypeBootLoaderConfig {
			return true
		}
		if pi.Type == PartTypeBootLoaderConfig && pj.Type == PartTypeBootLoaderConfig {
			return pi.Name < pj.Name
		}
		return false
	}
	// Then app.
	if pi.Type == PartTypeApp || pj.Type == PartTypeApp {
		if pi.Type == PartTypeApp && pj.Type != PartTypeApp {
			return true
		}
		if pi.Type == PartTypeApp && pj.Type == PartTypeApp {
			return pi.Name < pj.Name
		}
		return false
	}
	// Then fs containers.
	if pi.Type == PartTypeFSContainer || pj.Type == PartTypeFSContainer {
		if pi.Type == PartTypeFSContainer && pj.Type != PartTypeFSContainer {
			return true
		}
		if pi.Type == PartTypeFSContainer && pj.Type == PartTypeFSContainer {
			return pi.Name < pj.Name
		}
		return false
	}
	// Then the rest, sorted by name.
	return pi.Name < pj.Name
}
