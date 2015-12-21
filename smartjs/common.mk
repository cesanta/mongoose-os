
GIT_REF     = $(shell git symbolic-ref HEAD | sed 's:refs/heads/::')
GIT_SHA     = $(shell git rev-parse HEAD | head -c 8)
DATE        = $(shell TZ=GMT date +"%Y%m%d-%H%M%S")
FW_VERSION  = $(DATE)/$(GIT_REF)/$(GIT_SHA)

MG_FEATURES_TINY = \
                   -DMG_DISABLE_MQTT \
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
                   -DMG_MAX_HTTP_HEADERS=20 \
                   -DMG_MAX_HTTP_REQUEST_SIZE=1024 \
                   -DMG_MAX_PATH=40 \
                   -DMG_MAX_HTTP_SEND_IOBUF=1024 \
                   -DNO_BSD_SOCKETS
