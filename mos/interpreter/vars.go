package interpreter

import (
	"fmt"
	"regexp"

	"github.com/cesanta/errors"
)

var (
	// Note: we opted to use ${foo} instead of {{foo}}, because {{foo}} needs to
	// be quoted in yaml, whereas ${foo} does not.
	varRegexp = regexp.MustCompile(`\$\{[^}]+\}`)
)

func ExpandVars(interp *MosInterpreter, s string, skipFailed bool) (string, error) {
	var errRet error
	result := varRegexp.ReplaceAllStringFunc(s, func(v string) string {
		expr := v[2 : len(v)-1]
		val, err := interp.EvaluateExprString(expr)
		if err != nil {
			if skipFailed {
				return v
			}
			errRet = errors.Annotatef(err, "expanding expressions in %q", s)
		}
		return val
	})
	return result, errRet
}

func ExpandVarsSlice(interp *MosInterpreter, slice []string, skipFailed bool) ([]string, error) {
	ret := []string{}
	for _, s := range slice {
		s, err := ExpandVars(interp, s, skipFailed)
		if err != nil {
			return nil, errors.Trace(err)
		}
		ret = append(ret, s)
	}
	return ret, nil
}

func WrapMosExpr(s string) string {
	return fmt.Sprintf("${%s}", s)
}
