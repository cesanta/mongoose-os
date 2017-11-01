// Copyright (c) 2014-2017 Cesanta Software Limited
// All rights reserved

package interpreter

import (
	"fmt"
	"regexp"

	"github.com/cesanta/errors"
)

var (
	regexpPart    = regexp.MustCompile(`\s+`)
	regexpString  = regexp.MustCompile(`^\"[^"]*\"$`)
	regexpDefined = regexp.MustCompile(`^defined\(\s*([^)]+)\s*\)$`)
)

// MosInterpreter can evaluate very simple expressions, see EvaluateExpr.
// Expressions are evaluated against enclosed MosVars.
type MosInterpreter struct {
	MVars *MosVars
}

func NewInterpreter(mVars *MosVars) *MosInterpreter {
	return &MosInterpreter{
		MVars: mVars,
	}
}

func (mi *MosInterpreter) Copy() *MosInterpreter {
	return &MosInterpreter{
		MVars: mi.MVars.Copy(),
	}
}

// EvaluateExpr can evaluate expressions of the following form:
//
//   - operand
//   - operand1 op operand2
//
// Where either operand can be a string like "foo", or an expression
// suitable for MosVars.GetVar, e.g. foo.bar.baz. Operation can be either
// == or !=.
//
// Examples:
//
//  - arch
//  - build_vars.FOO_BAR == "foo"
//  - "bar"
//
// In the future it will be hopefully refactored into a proper expression
// parsing and evaluation, but so far it's a quick hack which solves the
// problem at hand.
func (mi *MosInterpreter) EvaluateExpr(expr string) (interface{}, error) {
	parts := regexpPart.Split(expr, -1)

	switch len(parts) {
	case 1:
		return mi.evaluatePart(parts[0])

	case 3:
		operand1, err := mi.evaluatePart(parts[0])
		if err != nil {
			return nil, errors.Trace(err)
		}

		op := parts[1]

		operand2, err := mi.evaluatePart(parts[2])
		if err != nil {
			return nil, errors.Trace(err)
		}

		operand1Str, ok := operand1.(string)
		if !ok {
			return nil, errors.Errorf("only strings are allowed, %T given (%s)", operand1, operand1)
		}

		operand2Str, ok := operand2.(string)
		if !ok {
			return nil, errors.Errorf("only strings are allowed, %T given (%s)", operand2, operand2)
		}

		switch op {
		case "==":
			return operand1Str == operand2Str, nil
		case "!=":
			return operand1Str != operand2Str, nil
		default:
			return nil, errors.Errorf("%q is not a valid operation", op)
		}

	default:
		return nil, errors.Errorf("can't parse the expression %s", expr)
	}
}

// EvaluateExprString calls EvaluateExpr and converts the result to string
func (mi *MosInterpreter) EvaluateExprString(expr string) (string, error) {
	val, err := mi.EvaluateExpr(expr)
	if err != nil {
		return "", errors.Trace(err)
	}

	return fmt.Sprintf("%v", val), nil
}

// EvaluateExprBool calls EvaluateExpr and if the result is boolean, then
// returns it; otherwise returns an error
func (mi *MosInterpreter) EvaluateExprBool(expr string) (bool, error) {
	val, err := mi.EvaluateExpr(expr)
	if err != nil {
		return false, errors.Trace(err)
	}

	valBool, ok := val.(bool)
	if !ok {
		return false, errors.Errorf("expected bool, got %T (%v)", val, val)
	}

	return valBool, nil
}

func (mi *MosInterpreter) evaluatePart(expr string) (interface{}, error) {
	if regexpString.MatchString(expr) {
		// Expression looks like a string
		return expr[1 : len(expr)-1], nil
	} else if subexprs := regexpDefined.FindStringSubmatch(expr); subexprs != nil {
		// Expression looks like "defined(foo)"
		_, ok := mi.MVars.GetVar(subexprs[1])
		return ok, nil
	} else {
		// Try to get variable value
		val, ok := mi.MVars.GetVar(expr)
		if !ok {
			return nil, errors.Errorf("failed to evaluate %s", expr)
		}

		return val, nil
	}
}
