package ucl

import (
	"bytes"
	"fmt"
	"unicode/utf16"
)

func jsonEscape(s string) string {
	r := bytes.NewBuffer(nil)
	for _, c := range s {
		switch c {
		case '"':
			r.WriteString(`\"`) // err is always nil
		case '\\':
			r.WriteString(`\\`) // err is always nil
		case '\b':
			r.WriteString(`\b`) // err is always nil
		case '\f':
			r.WriteString(`\f`) // err is always nil
		case '\n':
			r.WriteString(`\n`) // err is always nil
		case '\r':
			r.WriteString(`\r`) // err is always nil
		case '\t':
			r.WriteString(`\t`) // err is always nil
		default:
			r.WriteRune(c) // err is always nil
		}
	}
	return r.String()
}

func isHexDigit(c rune) bool {
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')
}

func hexDigitValue(c rune) rune {
	switch {
	case c >= '0' && c <= '9':
		return c - '0'
	case c >= 'a' && c <= 'f':
		return c - 'a' + 10
	case c >= 'A' && c <= 'F':
		return c - 'A' + 10
	}
	return '\uFFFD'
}

func jsonUnescape(s string) (string, error) {
	r := bytes.NewBuffer(nil)
	start := 0
	const (
		RecordStart             = iota // Beginning of the string or right after the escape sequence.
		Regular                        // After a regular unescaped character.
		AfterBackslash                 // After a backslash.
		AfterU                         // After \u, a separate counter is used to eat exactly 4 next characters.
		AfterSurrogate                 // After \u-escaped first part of surrogate pair.
		AfterSurrogateBackslash        // After \ while parsing second half of surrogate pair.
		AfterSurrogateU                // After \u while parsing second half of surrogate pair.
	)
	state := RecordStart
	var ucount uint
	var first, u rune
	// Iteration over a string interpretes it as UTF-8 and produces Unicode
	// runes. i is the index of the first byte of the rune, c is the rune.
	for i, c := range s {
		switch state {
		case RecordStart:
			switch c {
			case '\\':
				state = AfterBackslash
			default:
				start = i
				state = Regular
			}
		case Regular:
			switch c {
			case '\\':
				r.WriteString(s[start:i]) // err is always nil
				state = AfterBackslash
			}
		case AfterBackslash:
			switch c {
			case '"':
				r.WriteString("\"") // err is always nil
				state = RecordStart
			case '\\':
				r.WriteString("\\") // err is always nil
				state = RecordStart
			case '/':
				r.WriteString("/") // err is always nil
				state = RecordStart
			case 'b':
				r.WriteString("\b") // err is always nil
				state = RecordStart
			case 'f':
				r.WriteString("\f") // err is always nil
				state = RecordStart
			case 'n':
				r.WriteString("\n") // err is always nil
				state = RecordStart
			case 'r':
				r.WriteString("\r") // err is always nil
				state = RecordStart
			case 't':
				r.WriteString("\t") // err is always nil
				state = RecordStart
			case 'u':
				ucount = 0
				u = 0
				state = AfterU
			default:
				return "", fmt.Errorf("invalid escape sequence %q at %d", c, i)
			}
		case AfterU:
			if !isHexDigit(c) {
				return "", fmt.Errorf("invalid hex digit %q at %d", c, i)
			}
			v := hexDigitValue(c)
			u |= v << (4 * (3 - ucount))
			ucount++
			if ucount == 4 {
				if utf16.IsSurrogate(u) {
					first = u
					state = AfterSurrogate
				} else {
					r.WriteRune(u) // err is always nil
					state = RecordStart
				}
			}
		case AfterSurrogate:
			if c != '\\' {
				return "", fmt.Errorf("expecting another escaped character after a first component of surrogate pair at %d", i)
			}
			state = AfterSurrogateBackslash
		case AfterSurrogateBackslash:
			if c != 'u' {
				return "", fmt.Errorf("expecting another escaped character after a first component of surrogate pair at %d", i)
			}
			u = 0
			ucount = 0
			state = AfterSurrogateU
		case AfterSurrogateU:
			if !isHexDigit(c) {
				return "", fmt.Errorf("invalid hex digit %q at %d", c, i)
			}
			v := hexDigitValue(c)
			u |= v << (4 * (3 - ucount))
			ucount++
			if ucount == 4 {
				if !utf16.IsSurrogate(u) {
					return "", fmt.Errorf("expecting another escaped surrogate character after a first component of surrogate pair at %d", i)
				}
				r.WriteRune(utf16.DecodeRune(first, u))
				state = RecordStart
			}
		}
	}
	if state == Regular {
		r.WriteString(s[start:len(s)]) // err is always nil
	}
	if state != Regular && state != RecordStart {
		return "", fmt.Errorf("incomplete escape sequence at the end of the string")
	}
	return r.String(), nil
}
