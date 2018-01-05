package schema

import (
	"fmt"
	"net/url"
	"regexp"

	json "github.com/cesanta/ucl"
)

var validType = map[string]bool{
	"array":   true,
	"boolean": true,
	"integer": true,
	"null":    true,
	"number":  true,
	"object":  true,
	"string":  true,
}

// ValidateDraft04Schema checks that v is a valid JSON schema.
func ValidateDraft04Schema(v json.Value) error {
	return validateDraft04Schema("#", v)
}

func validateDraft04Schema(path string, v json.Value) error {
	switch v := v.(type) {
	case *json.Object:
		s, found := v.Lookup("$ref")
		if found {
			return validateURI(path+"/$ref", s)
		}
		validators := map[string]func(string, json.Value) error{
			"type":                 validateType,
			"id":                   validateURI,
			"$schema":              validateURI,
			"title":                validateString,
			"description":          validateString,
			"multipleOf":           validateMultipleOf,
			"maximum":              validateNumber,
			"minimum":              validateNumber,
			"exclusiveMaximum":     validateBoolean,
			"exclusiveMinimum":     validateBoolean,
			"minLength":            validatePositiveInteger,
			"maxLength":            validatePositiveInteger,
			"pattern":              validatePattern,
			"additionalItems":      validateBoolOrSchema,
			"items":                validateItems,
			"maxItems":             validatePositiveInteger,
			"minItems":             validatePositiveInteger,
			"uniqueItems":          validateBoolean,
			"maxProperties":        validatePositiveInteger,
			"minProperties":        validatePositiveInteger,
			"required":             validateStringArray,
			"additionalProperties": validateBoolOrSchema,
			"definitions":          validateSchemaCollection,
			"properties":           validateSchemaCollection,
			"patternProperties":    validateSchemaCollection,
			"dependencies":         validateDependencies,
			"enum":                 validateEnum,
			"allOf":                validateSchemaArray,
			"anyOf":                validateSchemaArray,
			"oneOf":                validateSchemaArray,
			"not":                  validateDraft04Schema,
		}
		for prop, validate := range validators {
			val, found := v.Lookup(prop)
			if !found {
				continue
			}
			err := validate(path+"/"+prop, val)
			if err != nil {
				return err
			}
		}
		_, a := v.Lookup("exclusiveMaximum")
		_, b := v.Lookup("maximum")
		if a && !b {
			return fmt.Errorf("%q: \"exclusiveMaximum\" requires \"maximum\" to be present")
		}
		_, a = v.Lookup("exclusiveMinimum")
		_, b = v.Lookup("minimum")
		if a && !b {
			return fmt.Errorf("%q: \"exclusiveMinimum\" requires \"minimum\" to be present")
		}
		return nil
	default:
		return fmt.Errorf("%q has invalid type, it needs to be an object", path)
	}
}

func validateType(path string, v json.Value) error {
	switch v := v.(type) {
	case *json.String:
		if !validType[v.Value] {
			return fmt.Errorf("%q: %q is not a valid type", path, v)
		}
		return nil
	case *json.Array:
		if len(v.Value) < 1 {
			return fmt.Errorf("%q must have at least 1 element", path)
		}
		for _, t := range v.Value {
			s, ok := t.(*json.String)
			if !ok {
				return fmt.Errorf("%q: each element must be a string", path)
			}
			if !validType[s.Value] {
				return fmt.Errorf("%q: %q is not a valid type", path, s)
			}
		}
		if err := uniqueItems(v); err != nil {
			return fmt.Errorf("%q: %s", path, err)
		}
		return nil
	default:
		return fmt.Errorf("%q must be a string or an array of strings", path)
	}
}

func isValidURI(s string) error {
	_, err := url.Parse(s)
	return err
}

func validateURI(path string, v json.Value) error {
	s, ok := v.(*json.String)
	if !ok {
		return fmt.Errorf("%q must be a string", path)
	}
	if err := isValidURI(s.Value); err != nil {
		return fmt.Errorf("%q must be a valid URI: %s", path, err)
	}
	return nil
}

func validateString(path string, v json.Value) error {
	_, ok := v.(*json.String)
	if !ok {
		return fmt.Errorf("%q must be a string", path)
	}
	return nil
}

func validateNumber(path string, v json.Value) error {
	_, ok := v.(*json.Number)
	if !ok {
		_, ok := v.(*json.Integer)
		if !ok {
			return fmt.Errorf("%q must be a number", path)
		}
	}
	return nil
}

func validateBoolean(path string, v json.Value) error {
	_, ok := v.(*json.Bool)
	if !ok {
		return fmt.Errorf("%q must be a boolean", path)
	}
	return nil
}

func validateMultipleOf(path string, v json.Value) error {
	switch n := v.(type) {
	case *json.Number:
		if n.Value <= 0 {
			return fmt.Errorf("%q must be > 0", path)
		}
	case *json.Integer:
		if n.Value <= 0 {
			return fmt.Errorf("%q must be > 0", path)
		}
	default:
		return fmt.Errorf("%q must be a number", path)
	}
	return nil
}

func validatePositiveInteger(path string, v json.Value) error {
	n, ok := v.(*json.Integer)
	if !ok {
		return fmt.Errorf("%q must be a number", path)
	}
	if n.Value <= 0 {
		return fmt.Errorf("%q must be > 0", path)
	}
	return nil
}

func validatePattern(path string, v json.Value) error {
	s, ok := v.(*json.String)
	if !ok {
		return fmt.Errorf("%q must be a string", path)
	}
	_, err := regexp.Compile(s.Value)
	if err != nil {
		return fmt.Errorf("%q must be a valid regexp: %s", path, err)
	}
	return nil
}

func validateBoolOrSchema(path string, v json.Value) error {
	switch v := v.(type) {
	case *json.Bool:
		return nil
	default:
		return validateDraft04Schema(path, v)
	}
}

func validateItems(path string, v json.Value) error {
	switch v := v.(type) {
	case *json.Array:
		return validateSchemaArray(path, v)
	default:
		return validateDraft04Schema(path, v)
	}
}

func validateSchemaArray(path string, v json.Value) error {
	a, ok := v.(*json.Array)
	if !ok {
		return fmt.Errorf("%q must be an array", path)
	}
	if len(a.Value) < 1 {
		return fmt.Errorf("%q must have at least 1 element", path)
	}
	for i, v := range a.Value {
		err := validateDraft04Schema(path+fmt.Sprintf("/[%d]", i), v)
		if err != nil {
			return err
		}
	}
	return nil
}

func validateStringArray(path string, v json.Value) error {
	a, ok := v.(*json.Array)
	if !ok {
		return fmt.Errorf("%q must be an array", path)
	}
	if len(a.Value) < 1 {
		return fmt.Errorf("%q must have at least 1 element", path)
	}
	for _, t := range a.Value {
		_, ok := t.(*json.String)
		if !ok {
			return fmt.Errorf("%q: each element must be a string", path)
		}
	}
	if err := uniqueItems(a); err != nil {
		return fmt.Errorf("%q: %s", path, err)
	}
	return nil
}

func validateSchemaCollection(path string, v json.Value) error {
	m, ok := v.(*json.Object)
	if !ok {
		return fmt.Errorf("%q must be an object", path)
	}
	for k, v := range m.Value {
		err := validateDraft04Schema(path+"/"+k.Value, v)
		if err != nil {
			return err
		}
	}
	return nil
}

func validateDependencies(path string, v json.Value) error {
	m, ok := v.(*json.Object)
	if !ok {
		return fmt.Errorf("%q must be an object", path)
	}
	for k, v := range m.Value {
		switch v := v.(type) {
		case *json.Object:
			err := validateDraft04Schema(path+"/"+k.Value, v)
			if err != nil {
				return err
			}
		case *json.Array:
			return validateStringArray(path+"/"+k.Value, v)
		default:
			return fmt.Errorf("%q must be an array or an object", path+"/"+k.Value)
		}
	}
	return nil
}

func validateEnum(path string, v json.Value) error {
	a, ok := v.(*json.Array)
	if !ok {
		return fmt.Errorf("%q must be an array", path)
	}
	if len(a.Value) < 1 {
		return fmt.Errorf("%q must have at least 1 element", path)
	}
	return nil
}
