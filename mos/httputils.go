package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"net/http"
	"net/url"

	"github.com/cesanta/errors"
	"github.com/golang/glog"
)

const (
	beOK APIStatus = -1 * iota
	beError
	beInvalidParametersError
	beDBError
	beInvalidLoginOrPasswordError
	beAccessDeniedError
	beLimitExceededError
	beInvaildEndpointError
	beNotFoundError
	beAlreadyExistsError
)

// APIStatus is the API status/error code, as returned in the SimpleAPIResponse
type APIStatus int

// SimpleAPIResponse is what most API calls return.
type SimpleAPIResponse struct {
	Status       APIStatus `json:"status"`
	ErrorMessage string    `json:"error_message"`
}

func callAPI(method string, args, res interface{}) error {
	ab, err := json.Marshal(args)
	if err != nil {
		return errors.Trace(err)
	}

	server, err := serverURL()
	if err != nil {
		return errors.Trace(err)
	}

	uri := fmt.Sprintf("%s/api/%s", server, method)
	glog.Infof("calling %q with: %s", uri, string(ab))
	req, err := http.NewRequest("POST", uri, bytes.NewReader(ab))
	if err != nil {
		return errors.Trace(err)
	}
	req.Header.Set("Content-Type", "application/json")
	req.SetBasicAuth(*user, *pass)

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		return errors.Trace(err)
	}
	defer resp.Body.Close()

	if err := json.NewDecoder(resp.Body).Decode(res); err != nil {
		return errors.Trace(err)
	}

	return nil
}

func serverURL() (*url.URL, error) {
	u, err := url.Parse(*server)
	if err != nil {
		return nil, errors.Trace(err)
	}

	// If given URL does not contain scheme, assume http
	if u.Scheme == "" {
		u.Scheme = "http"
	}
	return u, nil
}
