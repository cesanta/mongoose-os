/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

package cc3220

import (
	"encoding/xml"
	"io/ioutil"
	"os"
	"path/filepath"
	"runtime"
	"sort"

	"cesanta.com/common/go/ourutil"
	"cesanta.com/mos/flash/cc32xx"
	"cesanta.com/mos/flash/common"
	"github.com/cesanta/errors"
	"github.com/golang/glog"
)

const (
	imageConfigFileName = "ImageConfig.xml"
)

// This is not documented anywhere, but examples can be found in
// ~/.SLImageCreator/projects/*/sl_image/ImageConfig.xml
type imageConfigXML struct {
	XMLName          xml.Name          `xml:"Root"`
	ImageBuilderProp *imageBuilderProp `xml:"ImageBuilderProp,omitempty"`
	CommandsSetList  *commandsSetList  `xml:"CommandsSetList,omitempty"`
}

type imageBuilderProp struct {
	IsTheDeviceSecure     bool                   `xml:"IsTheDeviceSecure,attr"`
	StorageCapacityBytes  int                    `xml:"StorageCapacityBytes,attr"`
	DevelopmentImage      *developmentImage      `xml:"DevelopmentImage,omitempty"`
	RetToFactoryFlagsList *retToFactoryFlagsList `xml:"RetToFactoryFlagsList,omitempty"`
}

type developmentImage struct {
	DevMacAddress string `xml:"DevMacAddress,omitempty"`
}

type retToFactoryFlagsList struct {
	RetToFactoryFlag []retToFactoryFlag
}

type retToFactoryFlag string

const (
	rtffRetToImageHost   retToFactoryFlag = "RET_TO_IMAGE_HOST"
	rtffRetToDefaultHost                  = "RET_TO_DEFAULT_HOST"
	rtffRetToImageGPIO                    = "RET_TO_IMAGE_GPIO"
)

type commandsSetList struct {
	CommandsSet []commandsSet
}

type commandsSet struct {
	Command []command
}

type command struct {
	CommandFormatStorage         *commandFormatStorage         `xml:"CommandFormatStorage,omitempty"`
	CommandWriteCertificateStore *commandWriteCertificateStore `xml:"CommandWriteCertificateStore,omitempty"`
	CommandWriteServicePack      *commandWriteServicePack      `xml:"CommandWriteServicePack,omitempty"`
	CommandWriteSystemFile       *commandWriteSystemFile       `xml:"CommandWriteSystemFile,omitempty"`
	CommandWriteFile             *commandWriteFile             `xml:"CommandWriteFile,omitempty"`
}

type commandFormatStorage struct {
	EraseStorage           bool `xml:"EraseStorage,attr"`
	SecurityAlertThreshold int  `xml:"SecurityAlertThreshold,attr"`
}

type commandWriteCertificateStore struct {
	SignatureFileName string `xml:"SignatureFileName,attr"`
	FileLocation      string
}

type commandWriteServicePack struct {
	ServicePackVersion string `xml:"ServicePackVersion,attr"`
	FileLocation       string
}

type commandWriteSystemFile struct {
	SystemFileId systemFileId
	FileLocation string
}

type systemFileId string

const (
	sfiIPConfig   systemFileId = "CONFIG_TYPE_IP_CONFIG"
	sfiDeviceName              = "CONFIG_TYPE_DEVICE_NAME"
	sfiAP                      = "CONFIG_TYPE_AP"
	sfiMode                    = "CONFIG_TYPE_MODE"
	sfiSTAConfig               = "CONFIG_TYPE_STA_CONFIG"
	sfiDHCPSrv                 = "CONFIG_TYPE_DHCP_SRV"
)

type commandWriteFile struct {
	CertificationFileName string `xml:"CertificationFileName,attr,omitempty"`
	SignatureFileName     string `xml:"SignatureFileName,attr,omitempty"`
	FileToken             int    `xml:"FileToken,attr,omitempty"`
	FileSystemName        string
	FileOpenFlagsList     *fileOpenFlagsList
	MaxFileSize           int
	FileLocation          string
}

type fileOpenFlagsList struct {
	FileOpenFlag []fileOpenFlag
}

type fileOpenFlag string

const (
	fofFailsafe        fileOpenFlag = "FILE_OPEN_FLAG_FAILSAFE"
	fofPublicRead                   = "FILE_OPEN_FLAG_PUBLIC_READ"
	fofPublicWrite                  = "FILE_OPEN_FLAG_PUBLIC_WRITE"
	fofSecure                       = "FILE_OPEN_FLAG_SECURE"
	fofStatic                       = "FILE_OPEN_FLAG_STATIC"
	fofVendor                       = "FILE_OPEN_FLAG_VENDOR"
	fofNoSignatureTest              = "FILE_OPEN_FLAG_NO_SIGNATURE_TEST"
)

func isKnownPartType(pt string) bool {
	return pt == cc32xx.PartTypeServicePack ||
		pt == cc32xx.PartTypeCABundle ||
		pt == cc32xx.PartTypeCertificate ||
		pt == cc32xx.PartTypeSLFile ||
		pt == cc32xx.PartTypeSLConfig ||
		pt == cc32xx.PartTypeBootLoader ||
		pt == cc32xx.PartTypeBootLoaderConfig ||
		pt == cc32xx.PartTypeApp ||
		pt == cc32xx.PartTypeFSContainer
}

func buildXMLConfigFromFirmwareBundle(fw *common.FirmwareBundle, storageCapacity int, mac cc32xx.MACAddress) (string, error) {
	imc := &imageConfigXML{
		ImageBuilderProp: &imageBuilderProp{
			StorageCapacityBytes: storageCapacity,
			IsTheDeviceSecure:    true,
			DevelopmentImage: &developmentImage{
				DevMacAddress: mac.String(),
			},
			RetToFactoryFlagsList: &retToFactoryFlagsList{
				RetToFactoryFlag: []retToFactoryFlag{
					rtffRetToImageHost,
					rtffRetToDefaultHost,
					rtffRetToImageGPIO,
				},
			},
		},
		CommandsSetList: &commandsSetList{
			CommandsSet: []commandsSet{
				commandsSet{
					Command: []command{
						command{
							CommandFormatStorage: &commandFormatStorage{
								EraseStorage:           false,
								SecurityAlertThreshold: 15,
							},
						},
					},
				},
			},
		},
	}

	parts := []*common.FirmwarePart{}
	for _, p := range fw.Parts {
		if isKnownPartType(p.Type) {
			parts = append(parts, p)
		}
	}
	sort.Sort(cc32xx.PartsByTypeAndName(parts))

	cc := imc.CommandsSetList.CommandsSet[0].Command
	for _, p := range parts {
		switch p.Type {
		case cc32xx.PartTypeServicePack:
			spfn, _, err := fw.GetPartDataFile(p.Name)
			if err != nil {
				return "", errors.Trace(err)
			}
			cc = append(cc, command{
				CommandWriteServicePack: &commandWriteServicePack{
					ServicePackVersion: "UCF_ROM",
					FileLocation:       spfn,
				},
			})
		case cc32xx.PartTypeApp:
			if p.CC32XXFileSignature == "" || p.CC32XXSigningCert == "" {
				return "", errors.Errorf("%s: app image must be signed and signing certificate (public key) provided", p.Name)
			}
			appfn, appfs, err := fw.GetPartDataFile(p.Name)
			if err != nil {
				return "", errors.Annotatef(err, "%s: failed to get app image data", p.Name)
			}
			sigfn, _, err := fw.GetPartDataFile(p.CC32XXFileSignature)
			if err != nil {
				return "", errors.Annotatef(err, "%s: failed to get signature for", p.Name)
			}
			if _, err := fw.GetPartData(p.CC32XXSigningCert); err != nil {
				return "", errors.Annotatef(err, "%s: failed to get signing certificate file for", p.Name)
			}
			if p.CC32XXFileAllocSize > appfs {
				appfs = p.CC32XXFileAllocSize
			}
			cc = append(cc, command{
				CommandWriteFile: &commandWriteFile{
					FileLocation:          appfn,
					MaxFileSize:           appfs,
					SignatureFileName:     sigfn,
					CertificationFileName: p.CC32XXSigningCert,
					FileSystemName:        cc32xx.BootFlashImgName,
					FileOpenFlagsList: &fileOpenFlagsList{
						FileOpenFlag: []fileOpenFlag{fofFailsafe, fofSecure},
					},
				},
			})
		case cc32xx.PartTypeCertificate:
			// Certificate files are in every way the same as regular files,
			// we just want them placed at the beginning.
			fallthrough
		case cc32xx.PartTypeFSContainer:
			// FS container is just another file on SLFS.
			fallthrough
		case cc32xx.PartTypeSLFile:
			fn, fs, err := fw.GetPartDataFile(p.Name)
			if err != nil {
				return "", errors.Annotatef(err, "%s: failed to get file data", p.Name)
			}
			if p.CC32XXFileAllocSize > fs {
				fs = p.CC32XXFileAllocSize
			}
			cc = append(cc, command{
				CommandWriteFile: &commandWriteFile{
					FileLocation:   fn,
					MaxFileSize:    fs,
					FileSystemName: p.Name,
				},
			})
		case cc32xx.PartTypeSLConfig:
			break
			sfid, err := systemFileIdFromString(p.Name)
			if err != nil {
				return "", errors.Annotatef(err, "%s", p.Name)
			}
			fn, _, err := fw.GetPartDataFile(p.Name)
			if err != nil {
				return "", errors.Annotatef(err, "%s: failed to get file data", p.Name)
			}
			cc = append(cc, command{
				CommandWriteSystemFile: &commandWriteSystemFile{
					FileLocation: fn,
					SystemFileId: sfid,
				},
			})
		case cc32xx.PartTypeCABundle:
			if p.CC32XXFileSignature == "" {
				return "", errors.Errorf("%s: CA bundle must be signed", p.Name)
			}
			cabfn, _, err := fw.GetPartDataFile(p.Name)
			if err != nil {
				return "", errors.Annotatef(err, "%s: failed to get CA bundle", p.Name)
			}
			sigfn, _, err := fw.GetPartDataFile(p.CC32XXFileSignature)
			if err != nil {
				return "", errors.Annotatef(err, "%s: failed to get signature for", p.Name)
			}
			cc = append(cc, command{
				CommandWriteCertificateStore: &commandWriteCertificateStore{
					FileLocation:      cabfn,
					SignatureFileName: sigfn,
				},
			})
		}
	}
	imc.CommandsSetList.CommandsSet[0].Command = cc

	xmlData, err := xml.MarshalIndent(imc, "", "  ")
	if err != nil {
		return "", errors.Annotatef(err, "failed to marshal XML")
	}

	td, _ := fw.GetTempDir()
	xmlfn := filepath.Join(td, imageConfigFileName)
	glog.Infof("Writing XML config to %q", xmlfn)
	if err := ioutil.WriteFile(xmlfn, xmlData, 0644); err != nil {
		return "", errors.Annotatef(err, "failed to write %s", imageConfigFileName)
	}

	return xmlfn, nil
}

func findBPIBinary() (string, error) {
	// TODO(rojer): Mac support
	ufPattern, bpiBinaryName := "", ""
	if runtime.GOOS == "windows" {
		ufPattern = filepath.Join("c:\\", "ti", "uniflash_*")
		bpiBinaryName = "BuildProgrammingImage.exe"
	} else {
		ufPattern = filepath.Join(os.Getenv("HOME"), "ti", "uniflash_*")
		bpiBinaryName = "BuildProgrammingImage"
	}
	matches, _ := filepath.Glob(ufPattern)
	if len(matches) == 0 {
		return "", errors.Errorf("UniFlash not found (tried %s). Please install UniFlash from http://processors.wiki.ti.com/index.php/Category:CCS_UniFlash", ufPattern)
	}
	// In case there are multiple versions installed, use the latest.
	sort.Strings(matches)
	ufDir := matches[len(matches)-1]
	glog.Infof("Found UniFlash installation in %q", ufDir)
	bpib := ""
	filepath.Walk(ufDir, func(path string, info os.FileInfo, err error) error {
		if filepath.Base(path) == bpiBinaryName {
			bpib = path
			return os.ErrInvalid // Stop the search.
		}
		return nil
	})
	var err error
	if bpib == "" {
		err = errors.Errorf("could not find BuildProgrammingImage binary (looked in %q)", ufDir)
	}
	return bpib, err
}

func buildUCFImageFromFirmwareBundle(fw *common.FirmwareBundle, bpiBinary string, mac cc32xx.MACAddress, storageCapacity int) (string, int, error) {

	cfgfn, err := buildXMLConfigFromFirmwareBundle(fw, storageCapacity, mac)
	if err != nil {
		return "", -1, errors.Annotatef(err, "failed to create UCF image builder config")
	}

	td, _ := fw.GetTempDir()

	if err := ourutil.RunCmd(ourutil.CmdOutOnError, bpiBinary, "-i", td, "-x", cfgfn); err != nil {
		return "", -1, errors.Annotatef(err, "%s failed", filepath.Base(bpiBinary))
	}

	ucfn := filepath.Join(td, "Output", "Programming.ucf")
	fi, err := os.Stat(ucfn)
	if err != nil {
		return "", -1, errors.Errorf("%s exited successfully but did not produce expected output (%s)", filepath.Base(bpiBinary), ucfn)
	}

	return ucfn, int(fi.Size()), nil
}

func systemFileIdFromString(s string) (systemFileId, error) {
	switch s {
	case string(sfiIPConfig):
		return sfiIPConfig, nil
	case string(sfiDeviceName):
		return sfiDeviceName, nil
	case string(sfiAP):
		return sfiAP, nil
	case string(sfiMode):
		return sfiMode, nil
	case string(sfiSTAConfig):
		return sfiSTAConfig, nil
	case string(sfiDHCPSrv):
		return sfiDHCPSrv, nil
	default:
		return systemFileId(""), errors.Errorf("unknown system file id %q", s)
	}
}
