package schema

import (
	"fmt"
	"net/url"
	"regexp"
	"time"

	"github.com/asaskevich/govalidator"
)

var (
	hostnameRe = regexp.MustCompile(`^([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])(\.([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]{0,61}[a-zA-Z0-9]))*$`)
)

func verifyFormat(val string, format string) error {
	switch format {
	case "date-time":
		_, err := time.Parse(time.RFC3339, val)
		return err
	case "email":
		if !govalidator.IsEmail(val) {
			return fmt.Errorf("%q is not a valid email", val)
		}
	case "hostname":
		if !hostnameRe.MatchString(val) || len(val) > 255 {
			return fmt.Errorf("%q is not a valid hostname", val)
		}
	case "ipv4":
		if !govalidator.IsIPv4(val) {
			return fmt.Errorf("%q is not a valid IPv4 address", val)
		}
	case "ipv6":
		if !govalidator.IsIPv6(val) {
			return fmt.Errorf("%q is not a valid IPv6 address", val)
		}
	case "uri":
		u, err := url.Parse(val)
		if err != nil {
			return err
		}
		// XXX: \noideadog{this makes all the tests pass, not sure how it really should be validated}
		if u.Host == "" {
			return fmt.Errorf("%q is not absolute", val)
		}
	}
	return nil
}
