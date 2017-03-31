package esp8266

var (
	FlashSizeToId = map[string]int{
		// +1, to distinguish from null-value
		"4m":     1,
		"2m":     2,
		"8m":     3,
		"16m":    4,
		"32m":    5,
		"16m-c1": 6,
		"32m-c1": 7,
		"32m-c2": 8,
	}

	FlashSizes = []int{524288, 262144, 1048576, 2097152, 4194304, 2097152, 4194304, 4194304}
)
