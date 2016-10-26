CFLAGS_EXTRA ?=
PYTHON ?= python
COMMON_V7_FEATURES = \
    -DV7_ENABLE__File__list=1 \
    -DV7_ENABLE__File__require=1 \
    -DV7_ENABLE__Function__bind=1 \
    -DV7_ENABLE__Function__call=1 \
    -DV7_ENABLE__Math=1 \
    -DV7_ENABLE__Math__ceil=1 \
    -DV7_ENABLE__Math__floor=1 \
    -DV7_ENABLE__Math__max=1 \
    -DV7_ENABLE__Math__min=1 \
    -DV7_ENABLE__Math__random=1 \
    -DV7_ENABLE__Math__round=1 \
    -DV7_ENABLE__Memory__stats=1 \
    -DV7_ENABLE__Proxy=1

MG_FEATURES_TINY = \
                   -DMG_DISABLE_HTTP_DIGEST_AUTH \
                   -DMG_DISABLE_MD5 \
                   -DMG_DISABLE_HTTP_KEEP_ALIVE \
                   -DMG_ENABLE_HTTP_SSI=0 \
                   -DMG_ENABLE_HTTP_STREAMING_MULTIPART

V ?=
ifeq ("$(V)","1")
Q :=
else
Q := @
endif
vecho := @echo " "

print-var:
	$(eval _VAL=$$($(VAR)))
	@echo $(_VAL)
