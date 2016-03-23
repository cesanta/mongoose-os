CFLAGS_EXTRA ?=

COMMON_V7_FEATURES = -DV7_ENABLE__File__require=1

MG_FEATURES_TINY = \
                   -DMG_DISABLE_JSON_RPC \
                   -DMG_DISABLE_COAP \
                   -DMG_DISABLE_SYNC_RESOLVER \
                   -DMG_DISABLE_HTTP_DIGEST_AUTH \
                   -DMG_DISABLE_MD5 \
                   -DMG_DISABLE_SOCKETPAIR \
                   -DMG_DISABLE_HTTP_KEEP_ALIVE \
                   -DMG_DISABLE_DAV_AUTH \
                   -DMG_DISABLE_CGI \
                   -DMG_DISABLE_SSI \
                   -DMG_ENABLE_HTTP_STREAMING_MULTIPART \
                   -DMG_MAX_HTTP_HEADERS=20 \
                   -DMG_MAX_HTTP_REQUEST_SIZE=1024 \
                   -DMG_MAX_PATH=40 \
                   -DMG_MAX_HTTP_SEND_MBUF=1024 \
                   -DMG_NO_BSD_SOCKETS

V ?=
ifeq ("$(V)","1")
Q :=
else
Q := @
endif
vecho := @echo " "
