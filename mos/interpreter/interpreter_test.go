// Copyright (c) 2014-2017 Cesanta Software Limited
// All rights reserved

package interpreter

import "testing"

type interpExpectString struct {
	expr   string
	result string
	err    string
}

type interpExpectBool struct {
	expr   string
	result bool
	err    string
}

func TestInterpreterBasic(t *testing.T) {
	mVars := NewMosVars()
	mVars.SetVar("foo", "foo_val")
	mVars.SetVar("bar.baz.boo", "boo_val")

	mi := NewInterpreter(mVars)

	es := []interpExpectString{
		interpExpectString{`"foo"`, "foo", ""},
		interpExpectString{`"foo" == "bar"`, "false", ""},
		interpExpectString{`"foo" != "bar"`, "true", ""},
		interpExpectString{`"foo" == "foo"`, "true", ""},
		interpExpectString{`"foo" == "foo"sdf`, "", "failed to evaluate \"foo\"sdf"},
		interpExpectString{`"" == ""`, "true", ""},

		interpExpectString{`foo`, "foo_val", ""},
		interpExpectString{`foo == "foo"`, "false", ""},
		interpExpectString{`foo == "foo_val"`, "true", ""},
		interpExpectString{`bar.baz.boo`, "boo_val", ""},
		interpExpectString{`bar.baz.booo`, "", "failed to evaluate bar.baz.booo"},
	}

	for _, v := range es {
		res, err := mi.EvaluateExprString(v.expr)

		errMsg := ""
		if err != nil {
			errMsg = err.Error()
		}

		if errMsg != v.err {
			t.Fatalf("expr %q: want error %q, got %q", v.expr, v.err, errMsg)
		}

		if res != v.result {
			t.Fatalf("expr %q: want result %q, got %q", v.expr, v.result, res)
		}
	}

	eb := []interpExpectBool{
		interpExpectBool{`"foo"`, false, "expected bool, got string (foo)"},
		interpExpectBool{`"foo" == "bar"`, false, ""},
		interpExpectBool{`"foo" != "bar"`, true, ""},
		interpExpectBool{`"foo" == "foo"`, true, ""},
		interpExpectBool{`"" == ""`, true, ""},

		interpExpectBool{`foo == "foo"`, false, ""},
		interpExpectBool{`foo == "foo_val"`, true, ""},
		interpExpectBool{`bar.baz.boo == "boo_val"`, true, ""},
		interpExpectBool{`bar.baz.booo`, false, "failed to evaluate bar.baz.booo"},
		interpExpectBool{`defined(foo)`, true, ""},
		interpExpectBool{`defined(bar.baz.boo)`, true, ""},
		interpExpectBool{`defined(bar.baz.booo)`, false, ""},
	}

	for _, v := range eb {
		res, err := mi.EvaluateExprBool(v.expr)

		errMsg := ""
		if err != nil {
			errMsg = err.Error()
		}

		if errMsg != v.err {
			t.Fatalf("expr %q: want error %q, got %q", v.expr, v.err, errMsg)
		}

		if res != v.result {
			t.Fatalf("expr %q: want result %t, got %t", v.expr, v.result, res)
		}
	}

}
