package rpccreds

import (
	"io/ioutil"
	"strings"

	"github.com/cesanta/errors"
	flag "github.com/spf13/pflag"
)

var (
	rpcCreds = flag.String("rpc-creds", "", `Either "username:passwd" or "@filename" which contains username:passwd`)
)

func GetRPCCreds() (username, passwd string, err error) {
	if len(*rpcCreds) > 0 && (*rpcCreds)[0] == '@' {
		filename := (*rpcCreds)[1:]
		data, err := ioutil.ReadFile(filename)
		if err != nil {
			return "", "", errors.Annotatef(err, "reading RPC creds file %s", filename)
		}

		return getRPCCredsFromString(strings.TrimSpace(string(data)))
	} else {
		return getRPCCredsFromString(*rpcCreds)
	}
}

func getRPCCredsFromString(s string) (username, passwd string, err error) {
	parts := strings.Split(s, ":")
	if len(parts) == 2 {
		return parts[0], parts[1], nil
	} else {
		// TODO(dfrank): handle the case with nothing or only username provided,
		// and prompt the user for the missing parts.

		return "", "", errors.Errorf("Failed to get username and password: wrong RPC creds spec")
	}
}
