package main

import (
	"context"
	"crypto/x509"
	"encoding/base64"
	"encoding/json"
	"encoding/pem"
	"hash/crc32"
	"io/ioutil"
	"os"
	"strconv"
	"strings"

	atcaService "cesanta.com/fw/defs/atca"
	"cesanta.com/mos/atca"
	"cesanta.com/mos/dev"
	"github.com/cesanta/errors"
	flag "github.com/spf13/pflag"
	yaml "gopkg.in/yaml.v2"
)

var (
	format   string
	writeKey string
)

func initATCAFlags() {
	if !extendedMode {
		return
	}
	flag.StringVar(&format, "format", "", "Config format, hex or json")
	flag.StringVar(&writeKey, "write-key", "", "Write key file")
}

func getFormat(f, fn string) string {
	f = strings.ToLower(f)
	if f == "" {
		fn := strings.ToLower(fn)
		if strings.HasSuffix(fn, ".yaml") || strings.HasSuffix(fn, ".yml") {
			f = "yaml"
		} else if strings.HasSuffix(strings.ToLower(fn), ".json") {
			f = "json"
		} else {
			f = "hex"
		}
	}
	return f
}

func atcaGetConfig(ctx context.Context, dc *dev.DevConn) error {
	fn := ""
	args := flag.Args()
	if len(args) == 2 {
		fn = args[1]
	}

	_, confData, cfg, err := atca.Connect(ctx, dc)
	if err != nil {
		return errors.Annotatef(err, "Connect")
	}

	f := getFormat(format, fn)

	var s []byte
	if f == "json" || f == "yaml" {
		if f == "json" {
			s, _ = json.MarshalIndent(cfg, "", "  ")
		} else {
			s, _ = yaml.Marshal(cfg)
		}
	} else if f == "hex" {
		s = atca.WriteHex(confData, 4)
	} else {
		return errors.Errorf("%s: format not specified and could not be guessed", fn)
	}

	if fn != "" {
		err = ioutil.WriteFile(fn, s, 0644)
		if err != nil {
			return errors.Trace(err)
		}
	} else {
		os.Stdout.Write(s)
	}

	return nil
}

func atcaSetConfig(ctx context.Context, dc *dev.DevConn) error {
	args := flag.Args()
	if len(args) < 2 {
		return errors.Errorf("config filename is required")
	}
	fn := args[1]

	data, err := ioutil.ReadFile(fn)
	if err != nil {
		return errors.Trace(err)
	}

	f := getFormat(format, fn)

	var confData []byte
	if f == "yaml" || f == "json" {
		var c atca.Config
		if f == "yaml" {
			err = yaml.Unmarshal(data, &c)
		} else {
			err = json.Unmarshal(data, &c)
		}
		if err != nil {
			return errors.Annotatef(err, "failed to decode %s as %s", fn, f)
		}

		confData, err = atca.WriteBinaryConfig(&c)
		if err != nil {
			return errors.Annotatef(err, "encode %s", fn)
		}
	} else if f == "hex" {
		confData = atca.ReadHex(data)
	} else {
		return errors.Errorf("%s: format not specified and could not be guessed", fn)
	}

	if len(confData) != atca.ConfigSize {
		return errors.Errorf("%s: expected %d bytes, got %d", fn, atca.ConfigSize, len(confData))
	}

	cl, _, currentCfg, err := atca.Connect(ctx, dc)
	if err != nil {
		return errors.Annotatef(err, "Connect")
	}

	if currentCfg.LockConfig == atca.LockModeLocked {
		return errors.Errorf("config zone is already locked")
	}

	b64c := base64.StdEncoding.EncodeToString(confData)
	cs := int64(crc32.ChecksumIEEE(confData))
	req := &atcaService.SetConfigArgs{
		Config: &b64c,
		Crc32:  &cs,
	}

	if *dryRun {
		reportf("This is a dry run, would have set the following config:\n\n"+
			"%s\n"+
			"SetConfig %s\n\n"+
			"Set --dry-run=false to confirm.",
			atca.WriteHex(confData, 4), atca.JSONStr(*req))
		return nil
	}

	if err = cl.SetConfig(ctx, req); err != nil {
		return errors.Annotatef(err, "SetConfig")
	}

	reportf("\nSetConfig successful.")

	return nil
}

func atcaLockZone(ctx context.Context, dc *dev.DevConn) error {
	args := flag.Args()
	if len(args) != 2 {
		return errors.Errorf("lock zone name is required (config or data)")
	}

	var zone atca.LockZone
	switch strings.ToLower(args[1]) {
	case "config":
		zone = atca.LockZoneConfig
	case "data":
		zone = atca.LockZoneData
	default:
		return errors.Errorf("invalid zone '%s'", args[1])
	}

	cl, _, _, err := atca.Connect(ctx, dc)
	if err != nil {
		return errors.Annotatef(err, "Connect")
	}

	zoneInt := int64(zone)
	req := &atcaService.LockZoneArgs{Zone: &zoneInt}

	if *dryRun {
		reportf("This is a dry run, would have sent the following request:\n\n"+
			"LockZone %s\n\n"+
			"Set --dry-run=false to confirm.", atca.JSONStr(req))
		return nil
	}

	if err = cl.LockZone(ctx, req); err != nil {
		return errors.Annotatef(err, "LockZone")
	}

	reportf("LockZone successful.")

	return nil
}

func atcaSetECCPrivateKey(slot int64, cfg *atca.Config, data []byte) (*atcaService.SetKeyArgs, error) {
	var keyData []byte

	rest := data
	for {
		var pb *pem.Block
		pb, rest = pem.Decode(rest)
		if pb != nil {
			if pb.Type != "EC PRIVATE KEY" {
				continue
			}
			eck, err := x509.ParseECPrivateKey(pb.Bytes)
			if err != nil {
				return nil, errors.Annotatef(err, "ParseECPrivateKey")
			}
			reportf("Parsed %s", pb.Type)
			keyData = eck.D.Bytes()
			break
		} else {
			keyData = atca.ReadHex(data)
			break
		}
	}

	if len(keyData) == atca.PrivateKeySize+1 && keyData[0] == 0 {
		// Copy-pasted from X509, chop off leading 0.
		keyData = keyData[1:]
	}

	if len(keyData) != atca.PrivateKeySize {
		return nil, errors.Errorf("expected %d bytes, got %d", atca.PrivateKeySize, len(keyData))
	}

	b64k := base64.StdEncoding.EncodeToString(keyData)
	isECC := true
	req := &atcaService.SetKeyArgs{Key: &b64k, Ecc: &isECC}

	if cfg.LockValue == atca.LockModeLocked {
		if cfg.SlotInfo[slot].SlotConfig.WriteConfig&0x4 == 0 {
			return nil, errors.Errorf(
				"data zone is locked and encrypted writes on slot %d "+
					"are not enabled, key cannot be set", slot)
		}
		wks := int64(cfg.SlotInfo[slot].SlotConfig.WriteKey)
		if writeKey == "" {
			return nil, errors.Errorf(
				"data zone is locked, --write-key for slot %d "+
					"is required to modify slot %d", wks, slot)
		}
		reportf("Data zone is locked, "+
			"will perform encrypted write using slot %d using %s", wks, writeKey)
		wKeyData, err := ioutil.ReadFile(writeKey)
		if err != nil {
			return nil, errors.Trace(err)
		}
		wKey := atca.ReadHex(wKeyData)
		if len(wKey) != atca.KeySize {
			return nil, errors.Errorf("%s: expected %d bytes, got %d", writeKey, atca.KeySize, len(wKey))
		}
		b64wk := base64.StdEncoding.EncodeToString(wKey)
		req.Wkslot = &wks
		req.Wkey = &b64wk
	}

	return req, nil
}

func atcaSetKey(ctx context.Context, dc *dev.DevConn) error {
	args := flag.Args()
	if len(args) != 3 {
		return errors.Errorf("slot number and key filename are required")
	}
	slot, err := strconv.ParseInt(args[1], 0, 64)
	if err != nil || slot < 0 || slot > 15 {
		return errors.Errorf("invalid slot number %q", args[1])
	}

	fn := args[2]

	data, err := ioutil.ReadFile(fn)
	if err != nil {
		return errors.Trace(err)
	}

	cl, _, cfg, err := atca.Connect(ctx, dc)
	if err != nil {
		return errors.Annotatef(err, "Connect")
	}

	if cfg.LockConfig != atca.LockModeLocked {
		return errors.Errorf("config zone must be locked got SetKey to work")
	}

	var req *atcaService.SetKeyArgs

	si := cfg.SlotInfo[slot]
	if slot < 8 && si.KeyConfig.Private && si.KeyConfig.KeyType == atca.KeyTypeECC {
		reportf("Slot %d is a ECC private key slot", slot)
		req, err = atcaSetECCPrivateKey(slot, cfg, data)
	} else {
		reportf("Slot %d is a non-ECC private key slot", slot)
		keyData := atca.ReadHex(data)
		if len(keyData) != atca.KeySize {
			return errors.Errorf("%s: expected %d bytes, got %d", fn, atca.KeySize, len(keyData))
		}
		b64k := base64.StdEncoding.EncodeToString(keyData)
		isECC := false
		req = &atcaService.SetKeyArgs{Key: &b64k, Ecc: &isECC}
	}

	if err != nil {
		return errors.Annotatef(err, fn)
	}

	keyData, _ := base64.StdEncoding.DecodeString(*req.Key)
	cs := int64(crc32.ChecksumIEEE(keyData))
	req.Slot = &slot
	req.Crc32 = &cs

	if *dryRun {
		reportf("This is a dry run, would have set the following key on slot %d:\n\n%s\n"+
			"SetKey %s\n\n"+
			"Set --dry-run=false to confirm.",
			slot, atca.WriteHex(keyData, 16), atca.JSONStr(*req))
		return nil
	}

	if err = cl.SetKey(ctx, req); err != nil {
		return errors.Annotatef(err, "SetKey")
	}

	reportf("SetKey successful.")

	return nil
}

func genCSR(csrTemplateFile string, slot int, cl atcaService.Service) error {
	reportf("Generating CSR using template from %s", csrTemplateFile)
	data, err := ioutil.ReadFile(csrTemplateFile)
	if err != nil {
		return errors.Trace(err)
	}

	var pb *pem.Block
	pb, _ = pem.Decode(data)
	if pb == nil {
		return errors.Errorf("%s: not a PEM file", csrTemplateFile)
	}
	if pb.Type != "CERTIFICATE REQUEST" {
		return errors.Errorf("%s: expected to find certificate, found %s", csrTemplateFile, pb.Type)
	}
	csrTemplate, err := x509.ParseCertificateRequest(pb.Bytes)
	if err != nil {
		return errors.Annotatef(err, "%s: failed to parse certificate", csrTemplateFile)
	}
	reportf("%s: public key type %d, signature type %d\n%s",
		csrTemplateFile, csrTemplate.PublicKeyAlgorithm, csrTemplate.SignatureAlgorithm,
		csrTemplate.Subject.ToRDNSequence())
	if csrTemplate.PublicKeyAlgorithm != x509.ECDSA ||
		csrTemplate.SignatureAlgorithm != x509.ECDSAWithSHA256 {
		return errors.Errorf("%s: wrong public key and/or signature type; "+
			"expected ECDSA(%d) and SHA256(%d), got %d %d",
			x509.ECDSA, x509.ECDSAWithSHA256, csrTemplate.PublicKeyAlgorithm,
			csrTemplate.SignatureAlgorithm)
	}
	signer := atca.NewSigner(context.Background(), cl, slot)
	csr, err := x509.CreateCertificateRequest(nil, csrTemplate, signer)
	if err != nil {
		return errors.Annotatef(err, "failed to create new CSR")
	}

	pem.Encode(os.Stdout, &pem.Block{Type: "CERTIFICATE REQUEST", Bytes: csr})
	return nil
}

func atcaGenKey(ctx context.Context, dc *dev.DevConn) error {
	args := flag.Args()
	if len(args) < 2 {
		return errors.Errorf("slot number is required")
	}
	slot, err := strconv.ParseInt(args[1], 0, 64)
	if err != nil || slot < 0 || slot > 15 {
		return errors.Errorf("invalid slot number %q", args[1])
	}

	csrTemplate := ""
	if len(args) == 3 {
		csrTemplate = args[2]
	}

	cl, _, _, err := atca.Connect(ctx, dc)
	if err != nil {
		return errors.Annotatef(err, "Connect")
	}

	req := &atcaService.GenKeyArgs{Slot: &slot}

	if *dryRun {
		reportf("This is a dry run, would have sent the following request:\n\n"+
			"GenKey %s\n\n"+
			"Set --dry-run=false to confirm.",
			atca.JSONStr(*req))
		return nil
	}

	r, err := cl.GenKey(ctx, req)
	if err != nil {
		return errors.Annotatef(err, "GenKey")
	}

	if r.Pubkey == nil {
		return errors.New("no public key in response")
	}

	keyData, err := base64.StdEncoding.DecodeString(*r.Pubkey)
	if err != nil {
		return errors.Annotatef(err, "failed to decode pub key data")
	}
	if len(keyData) != atca.PublicKeySize {
		return errors.Errorf("expected %d bytes, got %d", atca.PublicKeySize, len(keyData))
	}

	reportf("Generated new ECC key on slot %d, public key:\n\n%s",
		slot, atca.WriteHex(keyData, 16))

	reportf("GenKey successful.")

	if csrTemplate != "" {
		return genCSR(csrTemplate, int(slot), cl)
	}

	return nil
}

func atcaGetPubKey(ctx context.Context, dc *dev.DevConn) error {
	args := flag.Args()
	if len(args) < 2 {
		return errors.Errorf("slot number is required")
	}
	slot, err := strconv.ParseInt(args[1], 0, 64)
	if err != nil || slot < 0 || slot > 15 {
		return errors.Errorf("invalid slot number %q", args[1])
	}

	csrTemplate := ""
	if len(args) == 3 {
		csrTemplate = args[2]
	}

	cl, _, _, err := atca.Connect(ctx, dc)
	if err != nil {
		return errors.Annotatef(err, "Connect")
	}

	req := &atcaService.GetPubKeyArgs{Slot: &slot}

	r, err := cl.GetPubKey(ctx, req)
	if err != nil {
		return errors.Annotatef(err, "GetPubKey")
	}

	if r.Pubkey == nil {
		return errors.New("no public key in response")
	}

	keyData, err := base64.StdEncoding.DecodeString(*r.Pubkey)
	if err != nil {
		return errors.Annotatef(err, "failed to decode pub key data")
	}
	if len(keyData) != atca.PublicKeySize {
		return errors.Errorf("expected %d bytes, got %d", atca.PublicKeySize, len(keyData))
	}

	reportf("Slot %d, public key:\n\n%s", slot, atca.WriteHex(keyData, 16))

	reportf("GetPubKey successful.")

	if csrTemplate != "" {
		return genCSR(csrTemplate, int(slot), cl)
	}

	return nil
}
