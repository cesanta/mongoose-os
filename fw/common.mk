CFLAGS_EXTRA ?=
PYTHON ?= python

MG_FEATURES_TINY = -DMG_MODULE_LINES \
                   -DMG_DISABLE_HTTP_KEEP_ALIVE \
                   -DMG_ENABLE_HTTP_SSI=0 \
                   -DMG_ENABLE_TUN=0 \
                   -DMG_ENABLE_HTTP_STREAMING_MULTIPART \
                   -DMG_SSL_IF_MBEDTLS_MAX_FRAGMENT_LEN=1024 \
                   -DMG_LOG_DNS_FAILURES \
                   -DMG_MAX_PATH=256 \

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
