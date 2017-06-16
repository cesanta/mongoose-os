package build

import (
	"errors"
	"testing"
)

type exprBoolExpect struct {
	expr string
	val  bool
	err  error
}

func TestEvaluate(t *testing.T) {
	m := &FWAppManifest{
		BuildVars: map[string]string{
			"FOO": "foo value",
			"BAR": "bar value",
		},
	}

	table := []exprBoolExpect{
		exprBoolExpect{`build_vars.FOO == "foo value"`, true, nil},
		exprBoolExpect{`build_vars.FOO == "foo value2"`, false, nil},
		exprBoolExpect{`build_vars.FOO == ""`, false, nil},
		exprBoolExpect{`  build_vars.BAR=="bar value"  `, true, nil},
		exprBoolExpect{`build_vars.BAZ == ""`, true, nil},
		exprBoolExpect{`build_vars.BAZ != ""`, false, nil},
		exprBoolExpect{`build_vars.FOO != ""`, true, nil},

		exprBoolExpect{
			`build_vars.FOO === "far value"`, false,
			errors.New(`"===" is not a valid operation`),
		},

		exprBoolExpect{
			`sdf.BAR == "bar value"`, false,
			errors.New(`"sdf.BAR == \"bar value\"" is not a valid expression`),
		},
	}

	for _, i := range table {
		v, err := m.EvaluateExprBool(i.expr)
		if v != i.val {
			t.Errorf("EvaluateExprBool(%q): want %v, got %v", i.expr, i.val, v)
		}
		if err == nil && i.err != nil || err != nil && i.err == nil || err != nil && i.err != nil && err.Error() != i.err.Error() {
			t.Errorf("EvaluateExprBool(%q) error: want %v, got %v", i.expr, i.err, err)
		}
	}

}
