
//line ucl.rl:1
package ucl

import (
	"fmt"
	"strconv"
	"strings"
)

type parserError struct {
	machine string
	offset int
	state int
}

func (e parserError) Error() string {
	return fmt.Sprintf("error parsing %s at char %d", e.machine, e.offset)
}


//line ucl.rl:34


//go:generate sh -c "ragel -Z -S number -V -p ucl.rl | dot -Tpng > number.png"

//line ucl.go:28
var _number_actions []byte = []byte{
	0, 2, 1, 0, 
}

var _number_key_offsets []byte = []byte{
	0, 0, 4, 7, 14, 16, 23, 27, 
	29, 36, 43, 
}

var _number_trans_keys []int32 = []int32{
	45, 48, 49, 57, 48, 49, 57, 43, 
	45, 46, 69, 101, 48, 57, 48, 57, 
	43, 69, 101, 45, 46, 48, 57, 43, 
	45, 48, 57, 48, 57, 43, 69, 101, 
	45, 46, 48, 57, 43, 45, 46, 69, 
	101, 48, 57, 
}

var _number_single_lengths []byte = []byte{
	0, 2, 1, 5, 0, 3, 2, 0, 
	3, 5, 0, 
}

var _number_range_lengths []byte = []byte{
	0, 1, 1, 1, 1, 2, 1, 1, 
	2, 1, 0, 
}

var _number_index_offsets []byte = []byte{
	0, 0, 4, 7, 14, 16, 22, 26, 
	28, 34, 41, 
}

var _number_indicies []byte = []byte{
	0, 2, 3, 1, 2, 3, 1, 1, 
	1, 5, 6, 6, 1, 4, 7, 1, 
	1, 6, 6, 1, 7, 4, 8, 8, 
	9, 1, 9, 1, 1, 1, 1, 1, 
	9, 4, 1, 1, 5, 6, 6, 3, 
	4, 1, 
}

var _number_trans_targs []byte = []byte{
	2, 0, 3, 9, 10, 4, 6, 5, 
	7, 8, 
}

var _number_trans_actions []byte = []byte{
	0, 0, 0, 0, 1, 0, 0, 0, 
	0, 0, 
}

const number_start int = 1
const number_first_final int = 10
const number_error int = 0

const number_en_main int = 1


//line ucl.rl:61


func parse_number(data []rune, p int, pe int) (Value, int, error) {
	var (
		cs int
		eof = pe
		ret Value
		start = p
	)
	_ = eof

//line ucl.go:100
	{
	cs = number_start
	}

//line ucl.rl:72

//line ucl.go:107
	{
	var _klen int
	var _trans int
	var _acts int
	var _nacts uint
	var _keys int
	if p == pe {
		goto _test_eof
	}
	if cs == 0 {
		goto _out
	}
_resume:
	_keys = int(_number_key_offsets[cs])
	_trans = int(_number_index_offsets[cs])

	_klen = int(_number_single_lengths[cs])
	if _klen > 0 {
		_lower := int(_keys)
		var _mid int
		_upper := int(_keys + _klen - 1)
		for {
			if _upper < _lower {
				break
			}

			_mid = _lower + ((_upper - _lower) >> 1)
			switch {
			case data[p] < _number_trans_keys[_mid]:
				_upper = _mid - 1
			case data[p] > _number_trans_keys[_mid]:
				_lower = _mid + 1
			default:
				_trans += int(_mid - int(_keys))
				goto _match
			}
		}
		_keys += _klen
		_trans += _klen
	}

	_klen = int(_number_range_lengths[cs])
	if _klen > 0 {
		_lower := int(_keys)
		var _mid int
		_upper := int(_keys + (_klen << 1) - 2)
		for {
			if _upper < _lower {
				break
			}

			_mid = _lower + (((_upper - _lower) >> 1) & ^1)
			switch {
			case data[p] < _number_trans_keys[_mid]:
				_upper = _mid - 2
			case data[p] > _number_trans_keys[_mid + 1]:
				_lower = _mid + 2
			default:
				_trans += int((_mid - int(_keys)) >> 1)
				goto _match
			}
		}
		_trans += _klen
	}

_match:
	_trans = int(_number_indicies[_trans])
	cs = int(_number_trans_targs[_trans])

	if _number_trans_actions[_trans] == 0 {
		goto _again
	}

	_acts = int(_number_trans_actions[_trans])
	_nacts = uint(_number_actions[_acts]); _acts++
	for ; _nacts > 0; _nacts-- {
		_acts++
		switch _number_actions[_acts-1] {
		case 0:
//line ucl.rl:27
 p--
 p++; goto _out
 
		case 1:
//line ucl.rl:41

		if strings.IndexAny(string(data[start:p]), ".eE") >= 0 {
			v, err := strconv.ParseFloat(string(data[start:p]), 64)
			if err != nil {
				return nil, -1, err
			}
			ret = &Number{Value: v}
		} else {
			v, err := strconv.ParseInt(string(data[start:p]), 10, 64)
			if err != nil {
				return nil, -1, err
			}
			ret = &Integer{Value: v}
		}
	
//line ucl.go:208
		}
	}

_again:
	if cs == 0 {
		goto _out
	}
	p++
	if p != pe {
		goto _resume
	}
	_test_eof: {}
	_out: {}
	}

//line ucl.rl:73

	if cs >= number_first_final {
		return ret, p, nil
	}
	return nil, -1, parserError{machine: "number", offset: p, state: cs}
}

//go:generate sh -c "ragel -Z -S object -V -p ucl.rl | dot -Tpng > object.png"

//line ucl.go:234
var _object_actions []byte = []byte{
	0, 1, 0, 1, 1, 1, 2, 1, 3, 
}

var _object_key_offsets []byte = []byte{
	0, 0, 5, 11, 15, 20, 24, 30, 
	35, 44, 50, 56, 62, 68, 
}

var _object_trans_keys []int32 = []int32{
	13, 32, 123, 9, 10, 13, 32, 34, 
	125, 9, 10, 34, 92, 32, 1114111, 13, 
	32, 58, 9, 10, 13, 32, 9, 10, 
	13, 32, 44, 125, 9, 10, 13, 32, 
	34, 9, 10, 34, 47, 92, 98, 102, 
	110, 114, 116, 117, 48, 57, 65, 70, 
	97, 102, 48, 57, 65, 70, 97, 102, 
	48, 57, 65, 70, 97, 102, 48, 57, 
	65, 70, 97, 102, 
}

var _object_single_lengths []byte = []byte{
	0, 3, 4, 2, 3, 2, 4, 3, 
	9, 0, 0, 0, 0, 0, 
}

var _object_range_lengths []byte = []byte{
	0, 1, 1, 1, 1, 1, 1, 1, 
	0, 3, 3, 3, 3, 0, 
}

var _object_index_offsets []byte = []byte{
	0, 0, 5, 11, 15, 20, 24, 30, 
	35, 45, 49, 53, 57, 61, 
}

var _object_indicies []byte = []byte{
	0, 0, 2, 0, 1, 2, 2, 3, 
	4, 2, 1, 6, 7, 5, 1, 8, 
	8, 9, 8, 1, 9, 9, 9, 10, 
	11, 11, 12, 4, 11, 1, 12, 12, 
	3, 12, 1, 5, 5, 5, 5, 5, 
	5, 5, 5, 13, 1, 14, 14, 14, 
	1, 15, 15, 15, 1, 16, 16, 16, 
	1, 5, 5, 5, 1, 1, 
}

var _object_trans_targs []byte = []byte{
	1, 0, 2, 3, 13, 3, 4, 8, 
	4, 5, 6, 6, 7, 9, 10, 11, 
	12, 
}

var _object_trans_actions []byte = []byte{
	0, 0, 0, 5, 0, 0, 7, 0, 
	0, 0, 3, 0, 0, 0, 0, 0, 
	0, 
}

var _object_from_state_actions []byte = []byte{
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 1, 
}

const object_start int = 1
const object_first_final int = 13
const object_error int = 0

const object_en_main int = 1


//line ucl.rl:113


func parse_object(data []rune, p int, pe int) (Value, int, error) {
	var (
		cs int
		eof = pe
		ret = &Object{Value: map[Key]Value{}}
		key Key
		start int
		index int
	)
	_ = eof


//line ucl.go:321
	{
	cs = object_start
	}

//line ucl.rl:127

//line ucl.go:328
	{
	var _klen int
	var _trans int
	var _acts int
	var _nacts uint
	var _keys int
	if p == pe {
		goto _test_eof
	}
	if cs == 0 {
		goto _out
	}
_resume:
	_acts = int(_object_from_state_actions[cs])
	_nacts = uint(_object_actions[_acts]); _acts++
	for ; _nacts > 0; _nacts-- {
		 _acts++
		switch _object_actions[_acts - 1] {
		case 0:
//line ucl.rl:27
 p--
 p++; goto _out
 
//line ucl.go:352
		}
	}

	_keys = int(_object_key_offsets[cs])
	_trans = int(_object_index_offsets[cs])

	_klen = int(_object_single_lengths[cs])
	if _klen > 0 {
		_lower := int(_keys)
		var _mid int
		_upper := int(_keys + _klen - 1)
		for {
			if _upper < _lower {
				break
			}

			_mid = _lower + ((_upper - _lower) >> 1)
			switch {
			case data[p] < _object_trans_keys[_mid]:
				_upper = _mid - 1
			case data[p] > _object_trans_keys[_mid]:
				_lower = _mid + 1
			default:
				_trans += int(_mid - int(_keys))
				goto _match
			}
		}
		_keys += _klen
		_trans += _klen
	}

	_klen = int(_object_range_lengths[cs])
	if _klen > 0 {
		_lower := int(_keys)
		var _mid int
		_upper := int(_keys + (_klen << 1) - 2)
		for {
			if _upper < _lower {
				break
			}

			_mid = _lower + (((_upper - _lower) >> 1) & ^1)
			switch {
			case data[p] < _object_trans_keys[_mid]:
				_upper = _mid - 2
			case data[p] > _object_trans_keys[_mid + 1]:
				_lower = _mid + 2
			default:
				_trans += int((_mid - int(_keys)) >> 1)
				goto _match
			}
		}
		_trans += _klen
	}

_match:
	_trans = int(_object_indicies[_trans])
	cs = int(_object_trans_targs[_trans])

	if _object_trans_actions[_trans] == 0 {
		goto _again
	}

	_acts = int(_object_trans_actions[_trans])
	_nacts = uint(_object_actions[_acts]); _acts++
	for ; _nacts > 0; _nacts-- {
		_acts++
		switch _object_actions[_acts-1] {
		case 1:
//line ucl.rl:85

		v, newp, err := parse_value(data, p, pe);
		if err != nil { return nil, -1, err };
		ret.Value[key] = v;
		p = ( newp) - 1

	
		case 2:
//line ucl.rl:92

		start = p
	
		case 3:
//line ucl.rl:96

		s, err := jsonUnescape(string(data[start+1:p]))
		if err != nil {
			return nil, -1, err
		}
		key = Key{Value: s, Index: index}
		index++
	
//line ucl.go:445
		}
	}

_again:
	if cs == 0 {
		goto _out
	}
	p++
	if p != pe {
		goto _resume
	}
	_test_eof: {}
	_out: {}
	}

//line ucl.rl:128

	if cs >= object_first_final {
		return ret, p, nil
	}
	return nil, -1, parserError{machine: "object", offset: p, state: cs}
}

//go:generate sh -c "ragel -Z -S array -V -p ucl.rl | dot -Tpng > array.png"

//line ucl.go:471
var _array_actions []byte = []byte{
	0, 1, 0, 1, 1, 
}

var _array_key_offsets []byte = []byte{
	0, 0, 5, 10, 16, 21, 
}

var _array_trans_keys []int32 = []int32{
	13, 32, 91, 9, 10, 13, 32, 93, 
	9, 10, 13, 32, 44, 93, 9, 10, 
	13, 32, 93, 9, 10, 
}

var _array_single_lengths []byte = []byte{
	0, 3, 3, 4, 3, 0, 
}

var _array_range_lengths []byte = []byte{
	0, 1, 1, 1, 1, 0, 
}

var _array_index_offsets []byte = []byte{
	0, 0, 5, 10, 16, 21, 
}

var _array_indicies []byte = []byte{
	0, 0, 2, 0, 1, 2, 2, 4, 
	2, 3, 5, 5, 6, 4, 5, 1, 
	6, 6, 1, 6, 3, 1, 
}

var _array_trans_targs []byte = []byte{
	1, 0, 2, 3, 5, 3, 4, 
}

var _array_trans_actions []byte = []byte{
	0, 0, 0, 3, 0, 0, 0, 
}

var _array_from_state_actions []byte = []byte{
	0, 0, 0, 0, 0, 1, 
}

const array_start int = 1
const array_first_final int = 5
const array_error int = 0

const array_en_main int = 1


//line ucl.rl:154


func parse_array(data []rune, p int, pe int) (Value, int, error) {
	var (
		cs int
		eof = pe
		ret = &Array{}
	)
	_ = eof


//line ucl.go:535
	{
	cs = array_start
	}

//line ucl.rl:165

//line ucl.go:542
	{
	var _klen int
	var _trans int
	var _acts int
	var _nacts uint
	var _keys int
	if p == pe {
		goto _test_eof
	}
	if cs == 0 {
		goto _out
	}
_resume:
	_acts = int(_array_from_state_actions[cs])
	_nacts = uint(_array_actions[_acts]); _acts++
	for ; _nacts > 0; _nacts-- {
		 _acts++
		switch _array_actions[_acts - 1] {
		case 0:
//line ucl.rl:27
 p--
 p++; goto _out
 
//line ucl.go:566
		}
	}

	_keys = int(_array_key_offsets[cs])
	_trans = int(_array_index_offsets[cs])

	_klen = int(_array_single_lengths[cs])
	if _klen > 0 {
		_lower := int(_keys)
		var _mid int
		_upper := int(_keys + _klen - 1)
		for {
			if _upper < _lower {
				break
			}

			_mid = _lower + ((_upper - _lower) >> 1)
			switch {
			case data[p] < _array_trans_keys[_mid]:
				_upper = _mid - 1
			case data[p] > _array_trans_keys[_mid]:
				_lower = _mid + 1
			default:
				_trans += int(_mid - int(_keys))
				goto _match
			}
		}
		_keys += _klen
		_trans += _klen
	}

	_klen = int(_array_range_lengths[cs])
	if _klen > 0 {
		_lower := int(_keys)
		var _mid int
		_upper := int(_keys + (_klen << 1) - 2)
		for {
			if _upper < _lower {
				break
			}

			_mid = _lower + (((_upper - _lower) >> 1) & ^1)
			switch {
			case data[p] < _array_trans_keys[_mid]:
				_upper = _mid - 2
			case data[p] > _array_trans_keys[_mid + 1]:
				_lower = _mid + 2
			default:
				_trans += int((_mid - int(_keys)) >> 1)
				goto _match
			}
		}
		_trans += _klen
	}

_match:
	_trans = int(_array_indicies[_trans])
	cs = int(_array_trans_targs[_trans])

	if _array_trans_actions[_trans] == 0 {
		goto _again
	}

	_acts = int(_array_trans_actions[_trans])
	_nacts = uint(_array_actions[_acts]); _acts++
	for ; _nacts > 0; _nacts-- {
		_acts++
		switch _array_actions[_acts-1] {
		case 1:
//line ucl.rl:140

		v, newp, err := parse_value(data, p, pe);
		if err != nil { return nil, -1, err };
		ret.Value = append(ret.Value, v)
		p = ( newp) - 1

	
//line ucl.go:644
		}
	}

_again:
	if cs == 0 {
		goto _out
	}
	p++
	if p != pe {
		goto _resume
	}
	_test_eof: {}
	_out: {}
	}

//line ucl.rl:166

	if cs >= array_first_final {
		return ret, p, nil
	}
	return nil, -1, parserError{machine: "array", offset: p, state: cs}
}

//go:generate sh -c "ragel -Z -S value -V -p ucl.rl | dot -Tpng > value.png"

//line ucl.go:670
var _value_actions []byte = []byte{
	0, 1, 0, 1, 1, 1, 2, 1, 3, 
	1, 4, 1, 5, 1, 6, 1, 7, 
	1, 8, 1, 9, 
}

var _value_key_offsets []byte = []byte{
	0, 0, 13, 19, 23, 32, 38, 44, 
	50, 56, 57, 58, 59, 60, 61, 62, 
	63, 64, 65, 66, 
}

var _value_trans_keys []int32 = []int32{
	13, 32, 34, 45, 91, 102, 110, 116, 
	123, 9, 10, 48, 57, 13, 32, 91, 
	123, 9, 10, 34, 92, 32, 1114111, 34, 
	47, 92, 98, 102, 110, 114, 116, 117, 
	48, 57, 65, 70, 97, 102, 48, 57, 
	65, 70, 97, 102, 48, 57, 65, 70, 
	97, 102, 48, 57, 65, 70, 97, 102, 
	97, 108, 115, 101, 117, 108, 108, 114, 
	117, 101, 
}

var _value_single_lengths []byte = []byte{
	0, 9, 4, 2, 9, 0, 0, 0, 
	0, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 0, 
}

var _value_range_lengths []byte = []byte{
	0, 2, 1, 1, 0, 3, 3, 3, 
	3, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 
}

var _value_index_offsets []byte = []byte{
	0, 0, 12, 18, 22, 32, 36, 40, 
	44, 48, 50, 52, 54, 56, 58, 60, 
	62, 64, 66, 68, 
}

var _value_indicies []byte = []byte{
	1, 1, 2, 3, 4, 5, 6, 7, 
	8, 1, 3, 0, 1, 1, 4, 8, 
	1, 0, 10, 11, 9, 0, 9, 9, 
	9, 9, 9, 9, 9, 9, 12, 0, 
	13, 13, 13, 0, 14, 14, 14, 0, 
	15, 15, 15, 0, 9, 9, 9, 0, 
	16, 0, 17, 0, 18, 0, 19, 0, 
	20, 0, 21, 0, 22, 0, 23, 0, 
	24, 0, 25, 0, 0, 
}

var _value_trans_targs []byte = []byte{
	0, 2, 3, 19, 19, 9, 13, 16, 
	19, 3, 19, 4, 5, 6, 7, 8, 
	10, 11, 12, 19, 14, 15, 19, 17, 
	18, 19, 
}

var _value_trans_actions []byte = []byte{
	1, 0, 11, 9, 7, 0, 0, 0, 
	5, 0, 13, 0, 0, 0, 0, 0, 
	0, 0, 0, 15, 0, 0, 19, 0, 
	0, 17, 
}

var _value_from_state_actions []byte = []byte{
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 3, 
}

var _value_eof_actions []byte = []byte{
	0, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 0, 
}

const value_start int = 1
const value_first_final int = 19
const value_error int = 0

const value_en_main int = 1


//line ucl.rl:222


func parse_value(data []rune, p int, pe int) (Value, int, error) {
	var (
		cs int
		eof = pe
		ret Value
		start int
	)


//line ucl.go:770
	{
	cs = value_start
	}

//line ucl.rl:233

//line ucl.go:777
	{
	var _klen int
	var _trans int
	var _acts int
	var _nacts uint
	var _keys int
	if p == pe {
		goto _test_eof
	}
	if cs == 0 {
		goto _out
	}
_resume:
	_acts = int(_value_from_state_actions[cs])
	_nacts = uint(_value_actions[_acts]); _acts++
	for ; _nacts > 0; _nacts-- {
		 _acts++
		switch _value_actions[_acts - 1] {
		case 1:
//line ucl.rl:27
 p--
 p++; goto _out
 
//line ucl.go:801
		}
	}

	_keys = int(_value_key_offsets[cs])
	_trans = int(_value_index_offsets[cs])

	_klen = int(_value_single_lengths[cs])
	if _klen > 0 {
		_lower := int(_keys)
		var _mid int
		_upper := int(_keys + _klen - 1)
		for {
			if _upper < _lower {
				break
			}

			_mid = _lower + ((_upper - _lower) >> 1)
			switch {
			case data[p] < _value_trans_keys[_mid]:
				_upper = _mid - 1
			case data[p] > _value_trans_keys[_mid]:
				_lower = _mid + 1
			default:
				_trans += int(_mid - int(_keys))
				goto _match
			}
		}
		_keys += _klen
		_trans += _klen
	}

	_klen = int(_value_range_lengths[cs])
	if _klen > 0 {
		_lower := int(_keys)
		var _mid int
		_upper := int(_keys + (_klen << 1) - 2)
		for {
			if _upper < _lower {
				break
			}

			_mid = _lower + (((_upper - _lower) >> 1) & ^1)
			switch {
			case data[p] < _value_trans_keys[_mid]:
				_upper = _mid - 2
			case data[p] > _value_trans_keys[_mid + 1]:
				_lower = _mid + 2
			default:
				_trans += int((_mid - int(_keys)) >> 1)
				goto _match
			}
		}
		_trans += _klen
	}

_match:
	_trans = int(_value_indicies[_trans])
	cs = int(_value_trans_targs[_trans])

	if _value_trans_actions[_trans] == 0 {
		goto _again
	}

	_acts = int(_value_trans_actions[_trans])
	_nacts = uint(_value_actions[_acts]); _acts++
	for ; _nacts > 0; _nacts-- {
		_acts++
		switch _value_actions[_acts-1] {
		case 0:
//line ucl.rl:23

		return nil, -1, fmt.Errorf("parse error at byte %d (state=%d)", p, cs)
	
		case 2:
//line ucl.rl:178

		v, newp, err := parse_object(data, p, pe);
		if err != nil { return nil, -1, err };
		ret = v;
		p = ( newp) - 1

	
		case 3:
//line ucl.rl:185

		v, newp, err := parse_array(data, p, pe);
		if err != nil { return nil, -1, err };
		ret = v;
		p = ( newp) - 1

	
		case 4:
//line ucl.rl:192

		v, newp, err := parse_number(data, p, pe)
		if err != nil { return nil, -1, err };
		ret = v;
		p = ( newp) - 1

	
		case 5:
//line ucl.rl:199

		start = p
	
		case 6:
//line ucl.rl:203

		s, err := jsonUnescape(string(data[start+1:p]))
		if err != nil {
			return nil, -1, err
		}
		ret = &String{Value: s}
	
		case 7:
//line ucl.rl:211
ret = &Bool{Value: false}
		case 8:
//line ucl.rl:212
ret = &Bool{Value: true}
		case 9:
//line ucl.rl:213
ret = &Null{}
//line ucl.go:925
		}
	}

_again:
	if cs == 0 {
		goto _out
	}
	p++
	if p != pe {
		goto _resume
	}
	_test_eof: {}
	if p == eof {
		__acts := _value_eof_actions[cs]
		__nacts := uint(_value_actions[__acts]); __acts++
		for ; __nacts > 0; __nacts-- {
			__acts++
			switch _value_actions[__acts-1] {
			case 0:
//line ucl.rl:23

		return nil, -1, fmt.Errorf("parse error at byte %d (state=%d)", p, cs)
	
//line ucl.go:949
			}
		}
	}

	_out: {}
	}

//line ucl.rl:234
	if cs >= value_first_final {
		return ret, p, nil
	}
	return nil, -1, parserError{machine: "value", offset: p, state: cs}
}

//go:generate sh -c "ragel -Z -S document -V -p ucl.rl | dot -Tpng > document.png"

//line ucl.go:966
var _document_actions []byte = []byte{
	0, 1, 0, 1, 1, 1, 2, 
}

var _document_key_offsets []byte = []byte{
	0, 0, 6, 
}

var _document_trans_keys []int32 = []int32{
	13, 32, 91, 123, 9, 10, 13, 32, 
	9, 10, 
}

var _document_single_lengths []byte = []byte{
	0, 4, 2, 
}

var _document_range_lengths []byte = []byte{
	0, 1, 1, 
}

var _document_index_offsets []byte = []byte{
	0, 0, 6, 
}

var _document_trans_targs []byte = []byte{
	1, 1, 2, 2, 1, 0, 2, 2, 
	2, 0, 
}

var _document_trans_actions []byte = []byte{
	0, 0, 5, 3, 0, 1, 0, 0, 
	0, 1, 
}

var _document_eof_actions []byte = []byte{
	0, 1, 0, 
}

const document_start int = 1
const document_first_final int = 2
const document_error int = 0

const document_en_main int = 1


//line ucl.rl:267


func parse_json(data []rune) (Value, int, error) {
	var (
		cs int
		p int
		pe int = len(data)
		eof int = len(data)
	)
	var ret Value


//line ucl.go:1026
	{
	cs = document_start
	}

//line ucl.rl:279

//line ucl.go:1033
	{
	var _klen int
	var _trans int
	var _acts int
	var _nacts uint
	var _keys int
	if p == pe {
		goto _test_eof
	}
	if cs == 0 {
		goto _out
	}
_resume:
	_keys = int(_document_key_offsets[cs])
	_trans = int(_document_index_offsets[cs])

	_klen = int(_document_single_lengths[cs])
	if _klen > 0 {
		_lower := int(_keys)
		var _mid int
		_upper := int(_keys + _klen - 1)
		for {
			if _upper < _lower {
				break
			}

			_mid = _lower + ((_upper - _lower) >> 1)
			switch {
			case data[p] < _document_trans_keys[_mid]:
				_upper = _mid - 1
			case data[p] > _document_trans_keys[_mid]:
				_lower = _mid + 1
			default:
				_trans += int(_mid - int(_keys))
				goto _match
			}
		}
		_keys += _klen
		_trans += _klen
	}

	_klen = int(_document_range_lengths[cs])
	if _klen > 0 {
		_lower := int(_keys)
		var _mid int
		_upper := int(_keys + (_klen << 1) - 2)
		for {
			if _upper < _lower {
				break
			}

			_mid = _lower + (((_upper - _lower) >> 1) & ^1)
			switch {
			case data[p] < _document_trans_keys[_mid]:
				_upper = _mid - 2
			case data[p] > _document_trans_keys[_mid + 1]:
				_lower = _mid + 2
			default:
				_trans += int((_mid - int(_keys)) >> 1)
				goto _match
			}
		}
		_trans += _klen
	}

_match:
	cs = int(_document_trans_targs[_trans])

	if _document_trans_actions[_trans] == 0 {
		goto _again
	}

	_acts = int(_document_trans_actions[_trans])
	_nacts = uint(_document_actions[_acts]); _acts++
	for ; _nacts > 0; _nacts-- {
		_acts++
		switch _document_actions[_acts-1] {
		case 0:
//line ucl.rl:23

		return nil, -1, fmt.Errorf("parse error at byte %d (state=%d)", p, cs)
	
		case 1:
//line ucl.rl:245

		v, newp, err := parse_object(data, p, pe);
		if err != nil { return nil, -1, err };
		ret = v;
		p = ( newp) - 1

	
		case 2:
//line ucl.rl:252

		v, newp, err := parse_array(data, p, pe);
		if err != nil { return nil, -1, err };
		ret = v;
		p = ( newp) - 1

	
//line ucl.go:1134
		}
	}

_again:
	if cs == 0 {
		goto _out
	}
	p++
	if p != pe {
		goto _resume
	}
	_test_eof: {}
	if p == eof {
		__acts := _document_eof_actions[cs]
		__nacts := uint(_document_actions[__acts]); __acts++
		for ; __nacts > 0; __nacts-- {
			__acts++
			switch _document_actions[__acts-1] {
			case 0:
//line ucl.rl:23

		return nil, -1, fmt.Errorf("parse error at byte %d (state=%d)", p, cs)
	
//line ucl.go:1158
			}
		}
	}

	_out: {}
	}

//line ucl.rl:280

	return ret, -1, nil
}
