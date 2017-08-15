package moscommon

type BuildStat struct {
	ArchOld     string `json:"arch"`
	Platform    string `json:"platform"`
	AppName     string `json:"app_name"`
	BuildTimeMS int    `json:"build_time_ms"`
}
