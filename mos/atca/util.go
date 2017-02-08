package atca

import (
	"context"
	"encoding/base64"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"hash/crc32"
	"os"
	"regexp"
	"strings"

	atcaService "cesanta.com/fw/defs/atca"
	"cesanta.com/mos/dev"
	"github.com/cesanta/errors"
)

const (
	KeyFilePrefix = "ATCA:"
)

func Connect(ctx context.Context, dc *dev.DevConn) (atcaService.Service, []byte, *Config, error) {
	cl := atcaService.NewClient(dc.RPC, "")

	r, err := cl.GetConfig(ctx)
	if err != nil {
		return nil, nil, nil, errors.Annotatef(err, "GetConfig")
	}

	if r.Config == nil {
		return nil, nil, nil, errors.New("no config data in response")
	}

	confData, err := base64.StdEncoding.DecodeString(*r.Config)
	if err != nil {
		return nil, nil, nil, errors.Annotatef(err, "failed to decode config data")
	}
	if len(confData) != ConfigSize {
		return nil, nil, nil, errors.Errorf("expected %d bytes, got %d", ConfigSize, len(confData))
	}

	if r.Crc32 == nil {
		return nil, nil, nil, errors.New("no checksum in response")
	}
	cs := crc32.ChecksumIEEE(confData)

	if cs != uint32(*r.Crc32) {
		return nil, nil, nil, errors.Errorf("checksum mismatch: expected 0x%08x, got 0x%08x", *r.Crc32, cs)
	}

	cfg, err := ParseBinaryConfig(confData)
	if err != nil {
		return nil, nil, nil, errors.Annotatef(err, "ParseBinaryConfig")
	}

	fmt.Fprintf(os.Stderr, "\nAECC508A rev 0x%x S/N 0x%s, config is %s, data is %s\n\n",
		cfg.Revision, hex.EncodeToString(cfg.SerialNum), strings.ToLower(string(cfg.LockConfig)),
		strings.ToLower(string(cfg.LockValue)))

	return cl, confData, cfg, nil
}

func WriteHex(data []byte, numPerLine int) []byte {
	s := ""
	for i := 0; i < len(data); {
		for j := 0; j < numPerLine && i < len(data); j++ {
			comma := ""
			if i < len(data)-1 {
				comma = ", "
			}
			s += fmt.Sprintf("0x%02x%s", data[i], comma)
			i++
		}
		s += "\n"
	}
	return []byte(s)
}

func ReadHex(data []byte) []byte {
	var result []byte
	hexByteRegex := regexp.MustCompile(`[0-9a-fA-F]{2}`)
	for _, match := range hexByteRegex.FindAllString(string(data), -1) {
		b, _ := hex.DecodeString(match)
		result = append(result, b[0])
	}
	return result
}

func JSONStr(v interface{}) string {
	bb, _ := json.MarshalIndent(v, "", "  ")
	return string(bb)
}
