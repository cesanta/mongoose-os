HUBSPOT_PAGE_ID=3511950787
WWW_SUBDIR=smartjs

.PHONY: generate

# TODO(imax): move to asciidoc.mk
upload: upload-hubspot

include ../../tools/asciidoc.mk

generate:
	$(MAKE) -C ../../cloud/doc/service generate

doc.html: generate
index.html: generate
index.hubspot.html: generate

smartjs.pdf: index.adoc
	asciidoctor -r asciidoctor-pdf \
		-a imagesdir=$(DOCGEN_DIR)/images -b pdf $< -o $@
