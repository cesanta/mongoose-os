package main

import (
	"bytes"
	"crypto"
	"crypto/ecdsa"
	"crypto/elliptic"
	"crypto/rand"
	"crypto/rsa"
	"crypto/x509"
	"encoding/pem"
	"fmt"
	"io/ioutil"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"time"

	"golang.org/x/net/context"

	"cesanta.com/common/go/lptr"
	atcaService "cesanta.com/fw/defs/atca"
	fwsys "cesanta.com/fw/defs/sys"
	"cesanta.com/mos/atca"
	"cesanta.com/mos/dev"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/credentials"
	"github.com/aws/aws-sdk-go/aws/defaults"
	"github.com/aws/aws-sdk-go/aws/endpoints"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/iot"
	"github.com/cesanta/errors"
	"github.com/go-ini/ini"
	"github.com/golang/glog"
	flag "github.com/spf13/pflag"
)

const (
	defaultCertType  = "ECDSA"
	rsaKeyBits       = 2048
	rsaCACert        = "data/ca-verisign-g5.crt.pem"
	ecCACert         = "data/ca-verisign-ecc-g2.crt.pem"
	i2cEnableOption  = "i2c.enable"
	atcaEnableOption = "sys.atca.enable"

	awsIoTPolicyNone        = "-"
	awsIoTPolicyMOS         = "mos-default"
	awsIoTPolicyMOSDocument = `{"Statement": [{"Effect": "Allow", "Action": "iot:*", "Resource": "*"}], "Version": "2012-10-17"}`
)

var (
	awsGGEnable   = false
	awsMQTTServer = ""
	awsRegion     = ""
	awsIoTPolicy  = ""
	awsIoTThing   = ""
	useATCA       = false
	atcaSlot      = 0
	certCN        = ""
	certType      = ""
	certFile      = ""
	keyFile       = ""
)

func init() {
	flag.BoolVar(&awsGGEnable, "aws-enable-greengrass", false, "Enable AWS Greengrass support")
	flag.StringVar(&awsMQTTServer, "aws-mqtt-server", "", "If not specified, calls DescribeEndpoint to get it from AWS")
	flag.StringVar(&awsRegion, "aws-region", "", "AWS region to use. If not specified, uses the default")
	flag.BoolVar(&useATCA, "use-atca", false, "Use ATCA (AECC508A) to store private key.")
	flag.IntVar(&atcaSlot, "atca-slot", 0, "When using ATCA, use this slot for key storage.")
	flag.StringVar(&certType, "cert-type", "", "Type of the key for new cert, RSA or ECDSA. Default is "+defaultCertType+".")
	flag.StringVar(&certCN, "cert-cn", "", "Common name for the certificate. By default uses device ID.")
	flag.StringVar(&awsIoTPolicy, "aws-iot-policy", "", "Attach this policy to the generated certificate")
	flag.StringVar(&awsIoTThing, "aws-iot-thing", "",
		"Attach the generated certificate to this thing. "+
			"By default uses device ID. Set to '-' to not attach certificate to any thing.")
	flag.StringVar(&certFile, "cert-file", "", "Certificate file name")
	flag.StringVar(&keyFile, "key-file", "", "Key file name")
	hiddenFlags = append(hiddenFlags, "aws-region", "use-atca", "atca-slot", "cert-cn")

	// Make sure we have our certs compiled in.
	MustAsset(rsaCACert)
	MustAsset(ecCACert)
}

func checkDevConfig(ctx context.Context, devConn *dev.DevConn, devConf *dev.DevConf) error {
	i2cEnabled, err := devConf.Get(i2cEnableOption)
	if err != nil {
		return errors.Annotatef(err, "failed ot get I2C enabled status")
	}
	atcaEnabled, err := devConf.Get(atcaEnableOption)
	if err != nil {
		return errors.Annotatef(err, "failed to get ATCA enabled status")
	}
	if i2cEnabled != "true" || atcaEnabled != "true" {
		if i2cEnabled != "true" {
			reportf("Enabling I2C...")
			devConf.Set(i2cEnableOption, "true")
		}
		if atcaEnabled != "true" {
			reportf("Enabling ATCA...")
			devConf.Set(atcaEnableOption, "true")
		}
		err = configSetAndSave(ctx, devConn, devConf)
		if err != nil {
			return errors.Annotatef(err, "failed to apply new configuration")
		}
		reportf("Reconnecting, please wait...")
		devConn.Disconnect(ctx)
		// Give the device time to reboot.
		time.Sleep(5 * time.Second)
		err = devConn.Connect(ctx, false)
		if err != nil {
			return errors.Annotatef(err, "failed to reconnect to the device")
		}
	}
	return nil
}

func getSvc() (*iot.IoT, error) {
	sess, err := session.NewSession()
	if err != nil {
		return nil, errors.Trace(err)
	}
	cfg := defaults.Get().Config

	if awsRegion == "" {
		output, err := getCommandOutput("aws", "configure", "get", "region")
		if err != nil {
			if cfg.Region == nil || *cfg.Region == "" {
				reportf("Failed to get default AWS region, please specify --aws-region")
				return nil, errors.New("AWS region not specified")
			} else {
				awsRegion = *cfg.Region
			}
		} else {
			awsRegion = strings.TrimSpace(output)
		}
	}

	reportf("AWS region: %s", awsRegion)
	cfg.Region = aws.String(awsRegion)

	creds, err := getAwsCredentials()
	if err != nil {
		// In UI mode, UI credentials are acquired in a different way.
		if isUI {
			return nil, errors.Trace(err)
		}
		creds, err = askForCreds()
		if err != nil {
			return nil, errors.Annotatef(err, "bad AWS credentials")
		}
	}
	cfg.Credentials = creds
	return iot.New(sess, cfg), nil
}

func getAwsCredentials() (*credentials.Credentials, error) {
	// Try environment first, fall back to shared.
	creds := credentials.NewEnvCredentials()
	_, err := creds.Get()
	if err != nil {
		creds = credentials.NewSharedCredentials("", "")
		_, err = creds.Get()
	}
	return creds, err
}

func getAWSRegions() []string {
	resolver := endpoints.DefaultResolver()
	partitions := resolver.(endpoints.EnumPartitions).Partitions()
	endpoints := partitions[0].Services()["iot"]
	var regions []string
	for k := range endpoints.Endpoints() {
		regions = append(regions, k)
	}
	sort.Strings(regions)
	return regions
}

func getAWSIoTThings() (string, error) {
	iotSvc, err := getSvc()
	if err != nil {
		return "", errors.Trace(err)
	}
	things, err := iotSvc.ListThings(&iot.ListThingsInput{})
	if err != nil {
		return "", errors.Trace(err)
	}
	return things.String(), nil
	// var policies []string
	// for _, p := range lpr.Policies {
	// 	policies = append(policies, *p.PolicyName)
	// }
	// sort.Strings(policies)
	// return []string{}, nil
}

func getAWSIoTPolicyNames() ([]string, error) {
	iotSvc, err := getSvc()
	if err != nil {
		return nil, errors.Trace(err)
	}
	lpr, err := iotSvc.ListPolicies(&iot.ListPoliciesInput{})
	if err != nil {
		return nil, errors.Trace(err)
	}
	var policies []string
	for _, p := range lpr.Policies {
		policies = append(policies, *p.PolicyName)
	}
	sort.Strings(policies)
	return policies, nil
}

func genCert(ctx context.Context, iotSvc *iot.IoT, devConn *dev.DevConn, devConf *dev.DevConf, devInfo *fwsys.GetInfoResult, cn string) (string, string, error) {
	var err error
	var pk crypto.Signer
	var pkFile, pkPEMBlockType string

	if awsIoTPolicy == "" {
		policies, err := getAWSIoTPolicyNames()
		if err != nil {
			return "", "", errors.Trace(err)
		}
		return "", "", errors.Errorf("--aws-iot-policy is not set. Please choose a security policy to attach "+
			"to the new certificate. --aws-iot-policy=%s will create a default permissive policy; or set --aws-iot-policy=%s to not attach any.\nExisting policies: %s",
			awsIoTPolicyMOS, awsIoTPolicyNone, strings.Join(policies, " "))
	}

	reportf("Generating certificate request, CN: %s", cn)
	var pkDERBytes []byte
	if useATCA {
		if atcaSlot < 0 || atcaSlot > 7 {
			return "", "", errors.Errorf("ATCA slot for private key must be between 0 and 7")
		}
		if certType != "" && strings.ToUpper(certType) != "ECDSA" {
			return "", "", errors.Errorf("ATCA only supports EC keys")
		}
		certType = "ECDSA"

		checkDevConfig(ctx, devConn, devConf)
		if err != nil {
			// Don't fail outright, but warn the user
			reportf("invalid device configuration: %s", err)
		}

		cl, _, atcaCfg, err := atca.Connect(ctx, devConn)
		if err != nil {
			return "", "", errors.Annotatef(err, "failed to connect to the crypto device")
		}
		if atcaCfg.LockConfig != atca.LockModeLocked || atcaCfg.LockValue != atca.LockModeLocked {
			return "", "", errors.Errorf("crypto chip is not fully configured; see step 2 here: https://mongoose-os.com/docs/overview/security.html#setup-guide")
		}
		if keyFile == "" {
			reportf("Generating new private key in slot %d", atcaSlot)
			_, err := cl.GenKey(ctx, &atcaService.GenKeyArgs{Slot: lptr.Int64(int64(atcaSlot))})
			if err != nil {
				return "", "", errors.Annotatef(err, "failed to generate private key in slot %d", atcaSlot)
			}
		} else {
			reportf("Using existing key in slot %d", atcaSlot)
		}
		pk = atca.NewSigner(ctx, cl, atcaSlot)
		pkFile = fmt.Sprintf("%s%d", atca.KeyFilePrefix, atcaSlot)
		pkPEMBlockType = "EC PRIVATE KEY"
	} else {
		if certType == "" {
			certType = defaultCertType
		}
		switch strings.ToUpper(certType) {
		case "RSA":
			pk, err = rsa.GenerateKey(rand.Reader, rsaKeyBits)
			if err != nil {
				return "", "", errors.Annotatef(err, "failed to generate EC private key")
			}
			pkPEMBlockType = "RSA PRIVATE KEY"
			pkDERBytes = x509.MarshalPKCS1PrivateKey(pk.(*rsa.PrivateKey))
		case "ECDSA":
			pk, err = ecdsa.GenerateKey(elliptic.P256(), rand.Reader)
			if err != nil {
				return "", "", errors.Annotatef(err, "failed to generate RSA private key")
			}
			pkPEMBlockType = "EC PRIVATE KEY"
			pkDERBytes, _ = x509.MarshalECPrivateKey(pk.(*ecdsa.PrivateKey))
		default:
			return "", "", errors.Errorf("unknown cert type %q", certType)
		}
		reportf("Generating %s private key locally", certType)
	}
	csrTmpl := &x509.CertificateRequest{}
	csrTmpl.Subject.CommonName = certCN
	csrData, err := x509.CreateCertificateRequest(rand.Reader, csrTmpl, pk)
	if err != nil {
		return "", "", errors.Annotatef(err, "failed to generate CSR")
	}
	pemBuf := bytes.NewBuffer(nil)
	pem.Encode(pemBuf, &pem.Block{Type: "CERTIFICATE REQUEST", Bytes: csrData})

	reportf("Asking AWS for a certificate...")
	ccResp, err := iotSvc.CreateCertificateFromCsr(&iot.CreateCertificateFromCsrInput{
		CertificateSigningRequest: aws.String(pemBuf.String()),
		SetAsActive:               aws.Bool(true),
	})
	if err != nil {
		return "", "", errors.Annotatef(err, "failed to obtain certificate from AWS")
	}
	glog.Infof("AWS response:\n%s", cn)
	reportf("Certificate ID: %s", *ccResp.CertificateId)
	reportf("Certificate ARN: %s", *ccResp.CertificateArn)
	certFile := fmt.Sprintf("aws-iot-%s.crt.pem", (*ccResp.CertificateId)[:10])
	if !useATCA {
		pkFile = fmt.Sprintf("aws-iot-%s.key.pem", (*ccResp.CertificateId)[:10])
		pkPEMBuf := bytes.NewBuffer(nil)
		pem.Encode(pkPEMBuf, &pem.Block{Type: pkPEMBlockType, Bytes: pkDERBytes})
		err = ioutil.WriteFile(pkFile, pkPEMBuf.Bytes(), 0400)
		if err != nil {
			return "", "", errors.Annotatef(err, "failed to write private key to %s")
		}
		reportf("Wrote private key to %s", pkFile)
	}
	certFileData := fmt.Sprintf("CN: %s\r\nID: %s\r\nARN: %s\r\n%s",
		certCN, *ccResp.CertificateId, *ccResp.CertificateArn, *ccResp.CertificatePem)
	err = ioutil.WriteFile(certFile, []byte(certFileData), 0400)
	if err != nil {
		return "", "", errors.Annotatef(err, "failed to write certificate to %s", pkFile)
	}
	reportf("Wrote certificate to %s", certFile)

	if awsIoTPolicy != awsIoTPolicyNone {
		if awsIoTPolicy == awsIoTPolicyMOS {
			policies, err := getAWSIoTPolicyNames()
			if err != nil {
				return "", "", errors.Trace(err)
			}
			found := false
			for _, p := range policies {
				if p == awsIoTPolicyMOS {
					found = true
					break
				}
			}
			if !found {
				reportf("Creating policy %q (%s)...", awsIoTPolicy, awsIoTPolicyMOSDocument)
				_, err := iotSvc.CreatePolicy(&iot.CreatePolicyInput{
					PolicyName:     aws.String(awsIoTPolicy),
					PolicyDocument: aws.String(awsIoTPolicyMOSDocument),
				})
				if err != nil {
					return "", "", errors.Annotatef(err, "failed to create policy")
				}
			}
		}
		reportf("Attaching policy %q to the certificate...", awsIoTPolicy)
		_, err := iotSvc.AttachPrincipalPolicy(&iot.AttachPrincipalPolicyInput{
			PolicyName: aws.String(awsIoTPolicy),
			Principal:  ccResp.CertificateArn,
		})
		if err != nil {
			return "", "", errors.Annotatef(err, "failed to attach policy")
		}
	}

	if awsIoTThing != "-" {
		if awsIoTThing == "" {
			awsIoTThing = cn
		}
		/* Try creating the thing, in case it doesn't exist. */
		_, err := iotSvc.CreateThing(&iot.CreateThingInput{
			ThingName: aws.String(awsIoTThing),
		})
		if err != nil && err.Error() != iot.ErrCodeResourceAlreadyExistsException {
			reportf("Error creating thing: %s", err)
			/*
			 * Don't fail right away, maybe we don't have sufficient permissions to
			 * create things but we can attach certs to existing things.
			 * If the thing does not exist, attaching will fail.
			 */
		}
		reportf("Attaching the certificate to %q...", awsIoTThing)
		_, err = iotSvc.AttachThingPrincipal(&iot.AttachThingPrincipalInput{
			ThingName: aws.String(awsIoTThing),
			Principal: ccResp.CertificateArn,
		})
		if err != nil {
			return "", "", errors.Annotatef(err, "failed to attach certificate to %q", awsIoTThing)
		}
	}

	return certFile, pkFile, nil
}

func storeCreds(ak, sak string) (*credentials.Credentials, error) {
	sc := &credentials.SharedCredentialsProvider{}
	_, _ = sc.Retrieve() // This will fail, but we only need it to initialize Filename
	if sc.Filename == "" {
		return nil, errors.New("Could not determine file for cred storage")
	}
	cf, err := ini.Load(sc.Filename)
	if err != nil {
		cf = ini.Empty()
	}
	cf.Section("default").NewKey("aws_access_key_id", ak)
	cf.Section("default").NewKey("aws_secret_access_key", sak)

	os.MkdirAll(filepath.Dir(sc.Filename), 0700)
	if err = cf.SaveTo(sc.Filename); err != nil {
		return nil, errors.Annotatef(err, "failed to save %s", sc.Filename)
	}
	os.Chmod(sc.Filename, 0600)

	reportf("Wrote credentials to: %s", sc.Filename)

	// This should now succeed.
	creds := credentials.NewSharedCredentials("", "")
	_, err = creds.Get()
	if err != nil {
		os.Remove(sc.Filename)
		return nil, errors.Annotatef(err, "invalid new credentials")
	}
	return creds, nil
}

func askForCreds() (*credentials.Credentials, error) {
	reportf("\r\nAWS credentials are missing. If this is the first time you are running this tool,\r\n" +
		"you will need to obtain AWS credentials from the AWS console as explained here:\r\n" +
		"  http://docs.aws.amazon.com/cli/latest/userguide/cli-chap-getting-set-up.html\r\n")
	yn := prompt("Would you like to enter them now [y/N]?")
	if strings.ToUpper(yn) != "Y" {
		return nil, errors.New("user declined to enter creds")
	}
	ak := prompt("Access Key ID:")
	sak := prompt("Secret Access Key:")
	return storeCreds(ak, sak)
}

func awsIoTSetup(ctx context.Context, devConn *dev.DevConn) error {
	iotSvc, err := getSvc()
	if err != nil {
		return err
	}

	reportf("Connecting to the device...")
	devInfo, err := devConn.GetInfo(ctx)
	if err != nil {
		return errors.Annotatef(err, "failed to connect to device")
	}
	devArch, devMAC := *devInfo.Arch, *devInfo.Mac
	reportf("  %s %s running %s", devArch, devMAC, *devInfo.App)
	devID := fmt.Sprintf("%s_%s", devArch, devMAC[6:])

	devConf, err := devConn.GetConfig(ctx)
	if err != nil {
		return errors.Annotatef(err, "failed to get config")
	}
	mqttConf, err := devConf.Get("mqtt")
	if err != nil {
		return errors.Annotatef(err, "failed to get device MQTT config")
	}
	reportf("Current MQTT config: %+v", mqttConf)

	awsGGConf, err := devConf.Get("aws.greengrass")
	if err == nil {
		reportf("Current AWS Greengrass config: %+v", awsGGConf)
	}

	if certCN == "" {
		certCN = devID
	}

	if certFile == "" {
		certFile, keyFile, err = genCert(ctx, iotSvc, devConn, devConf, devInfo, certCN)
		if err != nil {
			return errors.Annotatef(err, "failed to generate certificate")
		}
	}

	reportf("Uploading certificate...")
	err = fsPutFile(ctx, devConn, certFile, filepath.Base(certFile))
	if err != nil {
		return errors.Annotatef(err, "failed to upload %s", filepath.Base(certFile))
	}

	if !strings.HasPrefix(keyFile, atca.KeyFilePrefix) {
		reportf("Uploading key...")
		err = fsPutFile(ctx, devConn, keyFile, filepath.Base(keyFile))
		if err != nil {
			return errors.Annotatef(err, "failed to upload %s", filepath.Base(keyFile))
		}
	}

	// ca.pem has both roots in it, so, for platforms other than CC3200, we can just use that.
	// CC3200 does not support cert bundles and will always require specific CA cert.
	caCertFile := "ca.pem"
	uploadCACert := false
	if strings.ToLower(*devInfo.Arch) == "cc3200" {
		caCertFile = rsaCACert
		uploadCACert = true
	}

	if uploadCACert {
		caCertData := MustAsset(caCertFile)
		reportf("Uploading CA certificate...")
		err = fsPutData(ctx, devConn, bytes.NewBuffer(caCertData), filepath.Base(caCertFile))
		if err != nil {
			return errors.Annotatef(err, "failed to upload %s", filepath.Base(caCertFile))
		}
	}

	settings := map[string]string{
		"mqtt.ssl_cert":    filepath.Base(certFile),
		"mqtt.ssl_key":     filepath.Base(keyFile),
		"mqtt.ssl_ca_cert": filepath.Base(caCertFile),
	}

	if awsMQTTServer == "" {
		// Get the value of mqtt.server from aws
		de, err := iotSvc.DescribeEndpoint(&iot.DescribeEndpointInput{})
		if err != nil {
			return errors.Annotatef(err, "aws iot describe-endpoint failed!")
		}
		settings["mqtt.server"] = fmt.Sprintf("%s:8883", *de.EndpointAddress)
	} else {
		settings["mqtt.server"] = awsMQTTServer
	}

	if useATCA {
		// ATECC508A makes ECDSA much faster than RSA.
		settings["mqtt.ssl_cipher_suites"] = "TLS-ECDHE-ECDSA-WITH-AES-128-GCM-SHA256"
	}

	// MQTT requires device.id to be set.
	devId, err := devConf.Get("device.id")
	if devId == "" {
		settings["device.id"] = certCN
	}

	if awsGGEnable && awsGGConf != "" {
		settings["aws.greengrass.enable"] = "true"
		settings["mqtt.enable"] = "false"
	} else {
		settings["mqtt.enable"] = "true"
	}

	for k, v := range settings {
		if err = devConf.Set(k, v); err != nil {
			return errors.Annotatef(err, "failed to set %s", k)
		}
	}

	mqttConf, _ = devConf.Get("mqtt")
	reportf("New MQTT config: %+v", mqttConf)

	err = configSetAndSave(ctx, devConn, devConf)
	if err != nil {
		return errors.Trace(err)
	}

	return nil
}
