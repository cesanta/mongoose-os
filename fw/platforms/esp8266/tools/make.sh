#!/bin/bash
#
# this script is useful if you install docker-toolbox on windows and don't have make
#

SRC_DIR="$(cd "$(dirname $(readlink -f $0))/../../../../" && pwd)"
ESP_MIOT_DIR="$(cd "$(dirname $(readlink -f $0))/../" && pwd)"
DIR="/cesanta"${ESP_MIOT_DIR##$SRC_DIR}
SDK_CAT=$(cat ${ESP_MIOT_DIR}/sdk.version)

docker run --rm -i -v /${SRC_DIR}:/cesanta ${SDK_CAT} \
		//bin/bash -c "\
			if [ -d /cesanta/v7 ] ; then make -C /cesanta/v7 v7.c ; fi && \
			if [ -d /cesanta/mongoose ] ; then make -C /cesanta/mongoose mongoose.c mongoose.h ; fi && \
			cd ${DIR} && \
			make -f Makefile.build clean all \
				&& python ./tools/showbreakdown.py \
		"


