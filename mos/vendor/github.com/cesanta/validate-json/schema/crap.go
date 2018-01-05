package schema

import (
	"net/url"
	"strings"

	json "github.com/cesanta/ucl"
)

func expandIdsAndRefsAndAddThemToLoader(scope *url.URL, v json.Value, loader *Loader) {
	// HERE BE DRAGONS
	// I couldn't make much sense out of the part of the spec about scopes, so
	// this might behave in a weird way.
	switch val := v.(type) {
	case *json.Object:
		ref, ok := val.Find("$ref").(*json.String)
		if ok {
			if scope != nil && !strings.HasPrefix(ref.Value, "#") {
				refUrl, err := url.Parse(ref.Value)
				if err != nil {
					return
				}
				refUrl = scope.ResolveReference(refUrl)
				ref.Value = refUrl.String()
			}
			return
		}
		id, ok := val.Find("id").(*json.String)
		if ok {
			idUrl, err := url.Parse(id.Value)
			if err != nil {
				return
			}
			if !idUrl.IsAbs() && scope != nil {
				idUrl = scope.ResolveReference(idUrl)
			}
			loader.AddAs(v, idUrl.String())
			scope = idUrl
		}
		for _, item := range val.Value {
			expandIdsAndRefsAndAddThemToLoader(scope, item, loader)
		}
	case *json.Array:
		for _, item := range val.Value {
			expandIdsAndRefsAndAddThemToLoader(scope, item, loader)
		}
	}
}
