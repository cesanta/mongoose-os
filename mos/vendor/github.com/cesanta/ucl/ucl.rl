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

%%{
	machine common;
	alphtype rune;

	action error {
		return nil, -1, fmt.Errorf("parse error at byte %d (state=%d)", fpc, cs)
	}

	action done { fhold; fbreak; }

	ws = [ \t\r\n];

	unescaped = (0x20..0x21 | 0x23..0x5B | 0x5D..0x10FFFF);
	char = (unescaped | "\\" . ([\"\\/bfnrt] | "u" . [0-9a-fA-F]{4}));
	string = ("\"" . char** . "\"");
}%%

//go:generate sh -c "ragel -Z -S number -V -p ucl.rl | dot -Tpng > number.png"
%%{
	machine number;
	include common;

	action end_number {
		if strings.IndexAny(string(data[start:fpc]), ".eE") >= 0 {
			v, err := strconv.ParseFloat(string(data[start:fpc]), 64)
			if err != nil {
				return nil, -1, err
			}
			ret = &Number{Value: v}
		} else {
			v, err := strconv.ParseInt(string(data[start:fpc]), 10, 64)
			if err != nil {
				return nil, -1, err
			}
			ret = &Integer{Value: v}
		}
	}

	int = "0" | ([1-9] . [0-9]*);
	main := "-"? . int . ("." . [0-9]+)? . ([eE] . [\+\-]? . [0-9]+)? (^[0-9eE\-\+.] @end_number @done);

	write data;
}%%

func parse_number(data []rune, p int, pe int) (Value, int, error) {
	var (
		cs int
		eof = pe
		ret Value
		start = p
	)
	_ = eof
%% write init;
%% write exec;

	if cs >= number_first_final {
		return ret, p, nil
	}
	return nil, -1, parserError{machine: "number", offset: p, state: cs}
}

//go:generate sh -c "ragel -Z -S object -V -p ucl.rl | dot -Tpng > object.png"
%%{
	machine object;
	include common;

	action parse_value {
		v, newp, err := parse_value(data, fpc, pe);
		if err != nil { return nil, -1, err };
		ret.Value[key] = v;
		fexec newp;
	}

	action start_tok {
		start = fpc
	}

	action end_key {
		s, err := jsonUnescape(string(data[start+1:fpc]))
		if err != nil {
			return nil, -1, err
		}
		key = Key{Value: s, Index: index}
		index++
	}

	key = string >start_tok @end_key;
	member = (key . ws* . ":" . ws* . (^ws >parse_value));

	object_content = (member . (ws* . "," . ws* . member)*);

	main := (ws* . "{" . ws* . object_content? . ws* . ("}" %*done));

	write data;
}%%

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

%% write init;
%% write exec;

	if cs >= object_first_final {
		return ret, p, nil
	}
	return nil, -1, parserError{machine: "object", offset: p, state: cs}
}

//go:generate sh -c "ragel -Z -S array -V -p ucl.rl | dot -Tpng > array.png"
%%{
	machine array;
	include common;

	action parse_value {
		v, newp, err := parse_value(data, fpc, pe);
		if err != nil { return nil, -1, err };
		ret.Value = append(ret.Value, v)
		fexec newp;
	}

	value = ^(ws | "]") >parse_value;

	array_content = (value . (ws* . "," . ws* . value)*);

	main := (ws* . "[" . ws* . array_content? . ws* . ("]" %*done));

	write data;
}%%

func parse_array(data []rune, p int, pe int) (Value, int, error) {
	var (
		cs int
		eof = pe
		ret = &Array{}
	)
	_ = eof

%% write init;
%% write exec;

	if cs >= array_first_final {
		return ret, p, nil
	}
	return nil, -1, parserError{machine: "array", offset: p, state: cs}
}

//go:generate sh -c "ragel -Z -S value -V -p ucl.rl | dot -Tpng > value.png"
%%{
	machine value;
	include common;

	action parse_object {
		v, newp, err := parse_object(data, fpc, pe);
		if err != nil { return nil, -1, err };
		ret = v;
		fexec newp;
	}

	action parse_array {
		v, newp, err := parse_array(data, fpc, pe);
		if err != nil { return nil, -1, err };
		ret = v;
		fexec newp;
	}

	action parse_number {
		v, newp, err := parse_number(data, fpc, pe)
		if err != nil { return nil, -1, err };
		ret = v;
		fexec newp;
	}

	action start_tok {
		start = fpc
	}

	action end_string {
		s, err := jsonUnescape(string(data[start+1:fpc]))
		if err != nil {
			return nil, -1, err
		}
		ret = &String{Value: s}
	}

	false = "false" @{ret = &Bool{Value: false}};
	true = "true" @{ret = &Bool{Value: true}};
	nullval = "null" @{ret = &Null{}};

	array = ws* . ("[" >parse_array);
	object = ws* . ("{" >parse_object);
	number = [\-0-9] >parse_number;

	main := (false | true | nullval | object | array | number | (string >start_tok @end_string)) %*done $!error;

	write data;
}%%

func parse_value(data []rune, p int, pe int) (Value, int, error) {
	var (
		cs int
		eof = pe
		ret Value
		start int
	)

%% write init;
%% write exec;
	if cs >= value_first_final {
		return ret, p, nil
	}
	return nil, -1, parserError{machine: "value", offset: p, state: cs}
}

//go:generate sh -c "ragel -Z -S document -V -p ucl.rl | dot -Tpng > document.png"
%%{
	machine document;
	include common;

	action parse_object {
		v, newp, err := parse_object(data, fpc, pe);
		if err != nil { return nil, -1, err };
		ret = v;
		fexec newp;
	}

	action parse_array {
		v, newp, err := parse_array(data, fpc, pe);
		if err != nil { return nil, -1, err };
		ret = v;
		fexec newp;
	}

	array = "[" >parse_array;
	object = "{" >parse_object;

	document = ws* . (object | array) . ws*;

	main := document $!error;

	write data;
}%%

func parse_json(data []rune) (Value, int, error) {
	var (
		cs int
		p int
		pe int = len(data)
		eof int = len(data)
	)
	var ret Value

%% write init;
%% write exec;

	return ret, -1, nil
}
