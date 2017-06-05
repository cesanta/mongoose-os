package esp8266

var (
	FlashSizeToId = map[string]int{
		// +1, to distinguish from null-value
		"4m":  1,
		"2m":  2,
		"8m":  3,
		"16m": 4,
		"32m": 5,
	}

	FlashSizes = map[int]int{
		0: 524288,
		1: 262144,
		2: 1048576,
		3: 2097152,
		4: 4194304,
	}
)
