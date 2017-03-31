package esp32

var (
	FlashSizeToId = map[string]int{
		// +1, to distinguish from null-value
		"8m":   1,
		"16m":  2,
		"32m":  3,
		"64m":  4,
		"128m": 7,
	}

	FlashSizes = []int{1048576, 2097152, 4194304, 8388608, 16777216}
)
