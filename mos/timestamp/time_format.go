package timestamp

import (
	"fmt"
	"strings"
	"time"
)

const (
	StampUnixMicro = "StampUnixMicro"
)

func ParseTimeStampFormatSpec(tsfSpec string) string {
	// Is it one of the constants?
	switch tsfSpec {
	case "true":
		fallthrough
	case "yes":
		fallthrough
	case "on":
		fallthrough
	case "%s.%f":
		return StampUnixMicro

	case "":
		fallthrough
	case "false":
		fallthrough
	case "no":
		fallthrough
	case "off":
		return ""

	case "UnixDate":
		return time.UnixDate
	case "RubyDate":
		return time.RubyDate
	case "RFC822":
		return time.RFC822
	case "RFC822Z":
		return time.RFC822Z
	case "RFC850":
		return time.RFC850
	case "RFC1123":
		return time.RFC1123
	case "RFC1123Z":
		return time.RFC1123Z
	case "RFC3339":
		return time.RFC3339
	case "RFC3339Nano":
		return time.RFC3339Nano
	case "Kitchen":
		return time.Kitchen
	case "Stamp":
		return time.Stamp
	case "StampMilli":
		return time.StampMilli
	case "StampMicro":
		return time.StampMicro
	case "StampNano":
		return time.StampNano
	}
	// Is it a strftime string? Replace all the % specifiers with Go's equivalents.
	if strings.Contains(tsfSpec, "%") {
		for strftimeSpec, goSpec := range formatConv {
			tsfSpec = strings.Replace(tsfSpec, strftimeSpec, goSpec, -1)
		}
		// Fall through, it's Go spec now.
	}
	// Assume Go spec format.
	return tsfSpec
}

func FormatTimestamp(ts time.Time, goFormat string) string {
	switch goFormat {
	case "":
		return ""
	case StampUnixMicro:
		return fmt.Sprintf("%d.%06d", ts.Unix(), ts.Nanosecond()/1000)
	default:
		return ts.Format(goFormat)
	}
}

// taken from time/format.go
var formatConv = map[string]string{
	"%A": "Monday",
	"%a": "Mon",
	"%B": "January",
	"%F": "2006-01-02",
	"%b": "Jan",
	"%d": "02",
	"%H": "15",
	"%I": "03",
	"%M": "04",
	"%m": "01",
	"%P": "PM",
	"%p": "pm",
	"%S": "05",
	"%T": "15:04:05",
	"%Y": "2006",
	"%y": "06",
	"%Z": "MST",
	"%z": "-0700",
}
