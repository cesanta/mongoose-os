package main

import (
	"encoding/json"
	"io/ioutil"

	"github.com/cesanta/errors"
)

type State struct {
	Versions map[string]*StateVersion `json:"versions"`
}

type StateVersion struct {
}

var (
	state State
)

func stateInit() error {
	// Try to read state from file, and if it succeeds, unmarshal json from it;
	// otherwise just leave state empty
	if data, err := ioutil.ReadFile(stateFilepath); err == nil {
		if err := json.Unmarshal(data, &state); err != nil {
			return errors.Trace(err)
		}
	}

	if state.Versions == nil {
		state.Versions = make(map[string]*StateVersion)
	}

	return nil
}

func GetStateForVersion(version string) *StateVersion {
	return state.Versions[version]
}

func SetStateForVersion(version string, stateVer *StateVersion) {
	state.Versions[version] = stateVer
}

func SaveState() error {
	data, err := json.MarshalIndent(&state, "", "  ")
	if err != nil {
		return errors.Trace(err)
	}

	if err := ioutil.WriteFile(stateFilepath, data, 0644); err != nil {
		return errors.Trace(err)
	}

	return nil
}
