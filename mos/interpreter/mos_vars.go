// Copyright (c) 2014-2017 Cesanta Software Limited
// All rights reserved

package interpreter

import (
	"encoding/json"
	"fmt"
	"strings"

	"cesanta.com/mos/build"
	"cesanta.com/mos/datamap"

	moscommon "cesanta.com/mos/common"
	"github.com/cesanta/errors"
	"github.com/golang/glog"
)

const (
	mVarOldMosVersion = "mos_version"
	mVarOldPathSuffix = "_path"
)

// MosVars is a wrapper around datamap.DataMap with get-fail-handler which
// provides some shortcut phantom values, e.g. it makes everything under
// "manifest" available at the top level, and also provides backward
// compatibility: maps "foo_bar_path" to "mos.modules.foo_bar.path" (and
// prints a warning if such old name is used).
//
// There are also a couple of helper functions to set mos-specific variables:
// SetModuleVars() and SetManifestVars.
type MosVars struct {
	data *datamap.DataMap
}

func NewMosVars() *MosVars {
	ret := &MosVars{
		data: datamap.New(getFailHandler),
	}
	return ret
}

func (mv *MosVars) SetVar(name string, value interface{}) {
	glog.Infof("Set '%s'='%s'", name, value)
	mv.data.Set(name, value)
}

func (mv *MosVars) GetVar(name string) (interface{}, bool) {
	return mv.data.Get(name)
}

// SetModuleVars populates "mos.modules.<moduleName>".
func SetModuleVars(mVars *MosVars, moduleName, path string) {
	mVars.SetVar(GetMVarNameModulePath(moduleName), path)
}

// SetManifestVars makes all variables from the given manifest available at the
// top level and under "manifest". Any previously existing manifest variables
// are removed.
func SetManifestVars(mVars *MosVars, manifest *build.FWAppManifest) error {
	data, err := json.Marshal(manifest)
	if err != nil {
		return errors.Trace(err)
	}

	manifestMap := map[string]interface{}{}

	if err := json.Unmarshal(data, &manifestMap); err != nil {
		return errors.Trace(err)
	}

	mVars.SetVar(GetMVarNameManifest(), manifestMap)

	return nil
}

// getFailHandler is called when DataMap.Get fails to get the value normally
func getFailHandler(dm *datamap.DataMap, name string) (interface{}, bool) {
	// Check if it's an old_style ..._path variable
	if strings.HasSuffix(name, mVarOldPathSuffix) {
		moduleName := name[:len(name)-len(mVarOldPathSuffix)]
		newName := GetMVarNameModulePath(moduleName)
		val, ok := dm.Get(newName)
		if ok {
			printMVarsDeprecationWarning(name, newName)
			return val, true
		}
	}

	// Check if it's an old_style mos_version variable
	if name == mVarOldMosVersion {
		if val, ok := dm.Get(GetMVarNameMosVersion()); ok {
			printMVarsDeprecationWarning(mVarOldMosVersion, GetMVarNameMosVersion())
			return val, true
		}
	}

	// Make "arch" and "mos.arch" and "platform" to be aliases of "mos.platform"
	if name == "arch" || name == GetMVarName(GetMVarNameMos(), "arch") || name == "platform" {
		return dm.Get(GetMVarNameMosPlatform())
	}

	// Make everything under "manifest" also available at the top level
	if !strings.HasPrefix(name, fmt.Sprint(GetMVarNameManifest(), ".")) {
		if val, ok := dm.Get(GetMVarName(GetMVarNameManifest(), name)); ok {
			return val, true
		}
	}

	return nil, false
}

func printMVarsDeprecationWarning(name, newName string) {
	// TODO(dfrank): uncomment
	// reportf("WARNING: %q is deprecated, please use %q instead", name, newName)
}

func GetMVarName(names ...string) string {
	return strings.Join(names, ".")
}

// GetMVarNameMos returns "mos"
func GetMVarNameMos() string {
	return GetMVarName("mos")
}

// GetMVarNameManifest returns "manifest"
func GetMVarNameManifest() string {
	return GetMVarName("manifest")
}

// GetMVarNameModule returns a string like "mos.modules.foo"
func GetMVarNameModule(moduleName string) string {
	return GetMVarName(GetMVarNameMos(), "modules", moscommon.IdentifierFromString(moduleName))
}

// GetMVarNameModulePath returns a string like "mos.modules.foo.path"
func GetMVarNameModulePath(moduleName string) string {
	return GetMVarName(GetMVarNameModule(moduleName), "path")
}

// GetMVarNameMosVersion returns "mos.version"
func GetMVarNameMosVersion() string {
	return GetMVarName(GetMVarNameMos(), "version")
}

// GetMVarNameMosPlatform returns "mos.platform"
func GetMVarNameMosPlatform() string {
	return GetMVarName(GetMVarNameMos(), "platform")
}
