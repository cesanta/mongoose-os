package state

import (
	"encoding/json"
	"io/ioutil"

	"cesanta.com/mos/common/paths"

	"github.com/cesanta/errors"
)

type State struct {
	Versions         map[string]*StateVersion `json:"versions"`
	OldDirsConverted bool                     `json:"old_dirs_converted"`
}

type StateVersion struct {
}

var (
	mosState State
)

func Init() error {
	// Try to read state from file, and if it succeeds, unmarshal json from it;
	// otherwise just leave state empty
	if data, err := ioutil.ReadFile(paths.StateFilepath); err == nil {
		if err := json.Unmarshal(data, &mosState); err != nil {
			return errors.Trace(err)
		}
	}

	if mosState.Versions == nil {
		mosState.Versions = make(map[string]*StateVersion)
	}

	return nil
}

func GetState() *State {
	return &mosState
}

func GetStateForVersion(version string) *StateVersion {
	return mosState.Versions[version]
}

func SetStateForVersion(version string, stateVer *StateVersion) {
	mosState.Versions[version] = stateVer
}

func SaveState() error {
	data, err := json.MarshalIndent(&mosState, "", "  ")
	if err != nil {
		return errors.Trace(err)
	}

	if err := ioutil.WriteFile(paths.StateFilepath, data, 0644); err != nil {
		return errors.Trace(err)
	}

	return nil
}
