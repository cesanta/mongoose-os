VALIDATE ?= validate-json # go get github.com/cesanta/validate-json
YAML2JSON ?= yaml-to-json # go get go.pedge.io/pkg/cmd/yaml-to-json
DEFS ?= ../../clubby/defs

YAML = $(wildcard *.service.yaml)
OUTS = $(foreach s,$(YAML),$(s:.yaml=))

.PHONY: all clean

all: $(foreach s,$(YAML),$(s:.yaml=))

%.service.json: %.service.yaml
	which $(YAML2JSON) || go install cesanta.com/vendor/go.pedge.io/pkg/cmd/yaml-to-json
	$(YAML2JSON) < $< > $@

# validate and leave with the final name
%.service: %.service.json
	which $(VALIDATE) || go install cesanta.com/vendor/github.com/cesanta/validate-json
	$(VALIDATE) --schema=$(DEFS)/service.schema --input=$<
	cp $< $@

clean:
	rm *.service || true
