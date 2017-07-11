package moscommon

type BuildStat struct {
	Arch        string `json:"arch"`
	AppName     string `json:"app_name"`
	BuildTimeMS int    `json:"build_time_ms"`
}
