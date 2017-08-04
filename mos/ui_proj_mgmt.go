package main

import (
	"encoding/base64"
	"io/ioutil"
	"net/http"
	"os"
	"path/filepath"
	"strings"
	"sync"
	"time"

	"github.com/cesanta/errors"
	"golang.org/x/net/context"

	"cesanta.com/common/go/ourio"
	"cesanta.com/mos/build"
	"cesanta.com/mos/common/paths"
	"cesanta.com/mos/interpreter"
	"cesanta.com/mos/version"
)

// So far, building is only possible from the current directory,
// so before building some app we need to change current dir, and ensure
// that other builds won't interfere
var buildMtx sync.Mutex

func getRootByProjectType(pt projectType) (string, error) {
	switch pt {
	case projectTypeApp:
		return paths.AppsDir, nil
	case projectTypeLib:
		return paths.LibsDir, nil
	}
	return "", errors.Errorf("invalid project type: %q", pt)
}

func getProjectRootPath(r *http.Request) (string, error) {
	pt := r.FormValue("type")
	rootDir := ""
	if pt == "" {
		return "", errors.Errorf("type is required")
	} else if pt == "app" {
		rootDir = paths.AppsDir
	} else if pt == "lib" {
		rootDir = paths.LibsDir
	} else {
		return "", errors.Errorf("type must be either app or lib")
	}
	return rootDir, nil
}

func getProjectPath(r *http.Request) (string, error) {
	pt := r.FormValue("type")
	rootDir, err := getProjectRootPath(r)
	if err != nil {
		return "", err
	}
	pname := r.FormValue("project")
	if pname == "" {
		return "", errors.Errorf("%s is required", pt)
	}
	return filepath.Join(rootDir, pname), nil
}

func getFilePath(r *http.Request) (string, error) {
	projectPath, err := getProjectPath(r)
	if err != nil {
		return "", errors.Trace(err)
	}

	filename := r.FormValue("filename")
	if filename == "" {
		return "", errors.Errorf("filename is required")
	}

	return filepath.Join(projectPath, filename), nil
}

func moveAppLibFile(pt projectType, r *http.Request) error {
	rootDir, err := getRootByProjectType(pt)
	if err != nil {
		return errors.Trace(err)
	}

	// get pname and filename from the query string
	// (we may have put it into saveAppLibFileParams as well, but just to make
	// it symmetric with the getAppLibFile, they are in they query string)
	pname := r.FormValue(string(pt))
	if pname == "" {
		return errors.Errorf("%s is required", pt)
	}

	filename := r.FormValue("filename")
	if filename == "" {
		return errors.Errorf("filename is required")
	}

	newFilename := r.FormValue("to")
	if newFilename == "" {
		return errors.Errorf(`"to" is required`)
	}

	err = os.Rename(
		filepath.Join(rootDir, pname, filename),
		filepath.Join(rootDir, pname, newFilename),
	)
	if err != nil {
		return errors.Trace(err)
	}

	return nil
}

func initProjectManagementEndpoints() {
	http.HandleFunc("/list-projects", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")

		ret := appLibList{}

		dirPath, err := getProjectRootPath(r)
		if err != nil {
			httpReply(w, "", err)
			return
		}

		files, err := ioutil.ReadDir(dirPath)
		if err != nil {
			// On non-existing path, just return an empty slice
			if os.IsNotExist(err) {
				httpReply(w, ret, nil)
				return
			}
			// On some other error, return an error
			httpReply(w, ret, err)
			return
		}

		interp := interpreter.NewInterpreter(newMosVars())

		for _, f := range files {
			name := f.Name()
			manifest, _, err := readManifest(filepath.Join(dirPath, name), nil, interp)
			if err == nil {
				ret[name] = manifest
			}
		}

		httpReply(w, ret, err)
	})

	http.HandleFunc("/duplicate-project", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		project_path, err := getProjectPath(r)
		if err != nil {
			httpReply(w, false, err)
			return
		}
		target_name := r.FormValue("target_name")
		if target_name == "" {
			httpReply(w, false, errors.Errorf("target_name is required"))
			return
		}
		target_path := filepath.Join(filepath.Dir(project_path), target_name)
		err = ourio.CopyDir(project_path, target_path, nil)
		httpReply(w, err == nil, err)
	})

	http.HandleFunc("/import-project", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")

		url := r.FormValue("url")
		if url == "" {
			httpReply(w, false, errors.Errorf("url is required"))
			return
			// return "", errors.Errorf("url is required")
		}

		dirPath, err := getProjectRootPath(r)
		if err != nil {
			httpReply(w, "", err)
			return
		}

		swmod := build.SWModule{
			Origin:  url,
			Version: version.GetMosVersion(),
		}
		target_name := r.FormValue("target_name")
		if target_name != "" {
			swmod.Name = target_name
		}

		_, err = swmod.PrepareLocalDir(dirPath, os.Stdout, true, "", *libsUpdateInterval)
		if err != nil {
			httpReply(w, "", err)
			return
		}

		name, err := swmod.GetName()

		httpReply(w, name, err)
	})

	http.HandleFunc("/p/ls", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		ret := []string{}
		projectPath, err := getProjectPath(r)
		if err != nil {
			httpReply(w, ret, err)
			return
		}

		err = filepath.Walk(projectPath, func(path string, fi os.FileInfo, _ error) error {
			// Ignore the root path, directory entries
			if path == projectPath || fi.IsDir() {
				return nil
			}
			// Strip the path to dir
			path = path[len(projectPath)+1:]
			// Ignore ".git"
			if strings.HasPrefix(path, ".git") {
				return nil
			}
			ret = append(ret, path)
			return nil
		})
		httpReply(w, ret, err)
	})

	http.HandleFunc("/app/mv", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")

		result := true

		err := moveAppLibFile(projectTypeApp, r)
		if err != nil {
			err = errors.Trace(err)
			result = false
		}

		httpReply(w, result, err)
	})

	http.HandleFunc("/lib/mv", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")

		result := true

		err := moveAppLibFile(projectTypeLib, r)
		if err != nil {
			err = errors.Trace(err)
			result = false
		}

		httpReply(w, result, err)
	})

	http.HandleFunc("/p/get", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		filePath, err := getFilePath(r)
		if err != nil {
			httpReply(w, "", err)
			return
		}

		data, err := ioutil.ReadFile(filePath)
		if err != nil {
			httpReply(w, "", err)
			return
		}

		httpReply(w, base64.StdEncoding.EncodeToString(data), nil)
	})

	http.HandleFunc("/p/set", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		filePath, err := getFilePath(r)

		if err != nil {
			httpReply(w, false, err)
			return
		}

		data := r.FormValue("data")
		decoded, err := base64.StdEncoding.DecodeString(data)
		if err != nil {
			httpReply(w, false, err)
			return
		}

		err = ioutil.WriteFile(filePath, decoded, 0755)
		httpReply(w, err == nil, err)
	})

	http.HandleFunc("/p/rm", func(w http.ResponseWriter, r *http.Request) {
		fullPath, err := getFilePath(r)
		if err == nil {
			err = os.RemoveAll(fullPath)
		}
		httpReply(w, err == nil, err)
	})

	http.HandleFunc("/app/getfwpath", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		appDir, err := getProjectPath(r)
		httpReply(w, filepath.Join(appDir, "build", "fw.zip"), err)
	})

	http.HandleFunc("/app/build", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")

		// So far, building is only possible from the current directory,
		// so before building some app we need to change current dir, and ensure
		// that other builds won't interfere
		buildMtx.Lock()
		defer buildMtx.Unlock()

		var err error

		pname := r.FormValue("app")
		if pname == "" {
			httpReply(w, false, errors.Errorf("app is required"))
			return
		}

		bParams := buildParams{
			Arch: r.FormValue("arch"),
		}

		ctx, cancel := context.WithTimeout(context.Background(), 20*time.Second)
		defer cancel()

		// Get current directory, and defer chdir-ing back to it
		prevDir, err := os.Getwd()
		if err != nil {
			err = errors.Trace(err)
			httpReply(w, false, err)
			return
		}
		defer func() {
			os.Chdir(prevDir)
		}()

		// Get app directory and chdir there
		appDir := filepath.Join(paths.AppsDir, pname)

		if err := os.Chdir(appDir); err != nil {
			err = errors.Trace(err)
			httpReply(w, false, err)
			return
		}

		// Build the firmware
		err = doBuild(ctx, &bParams)
		if err != nil {
			err = errors.Trace(err)
			httpReply(w, false, err)
			return
		}

		httpReply(w, true, err)
	})
}
