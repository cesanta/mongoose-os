FROM ubuntu:bionic

RUN DEBIAN_FRONTEND=noninteractive apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y -q \
      apt-utils autoconf bison build-essential flex gawk gdb-multiarch git gperf help2man \
      libexpat-dev libncurses5-dev libtool-bin \
      python python-dev python-git python-pyelftools python-serial python-six python-yaml \
      python3 python3-dev python3-git python3-pyelftools python3-serial python3-six python3-yaml \
      software-properties-common texinfo unzip wget zip && \
    apt-get clean

RUN cd /tmp && \
    git clone https://github.com/rojer/fsync-stub && \
    cd /tmp/fsync-stub && ./install.sh && \
    rm -rf /tmp/fsync-stub

RUN useradd -d /opt/Espressif -m -s /bin/bash user && chown -R user /opt

ENV PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/opt/Espressif/esp-open-sdk/xtensa-lx106-elf/bin
ADD ct_path.sh /etc/profile.d

USER user

ARG DOCKER_TAG
RUN git clone -b $DOCKER_TAG https://github.com/cesanta/esp-open-sdk /opt/Espressif/esp-open-sdk && \
    cd /opt/Espressif/esp-open-sdk && \
    git submodule update --init && \
    nice make && rm -rf crosstool-NG

USER root

RUN mkdir -p /opt/Espressif/crosstool-NG/builds/ && \
    ln -s /opt/Espressif/esp-open-sdk/xtensa-lx106-elf /opt/Espressif/crosstool-NG/builds

# Set up non-OS SDK
RUN ln -s /opt/Espressif/esp-open-sdk/sdk /opt/Espressif/ESP8266_NONOS_SDK

RUN cd /opt/Espressif/ESP8266_NONOS_SDK/lib && \
    xtensa-lx106-elf-ar -x libmain.a && \
    xtensa-lx106-elf-objcopy --weaken mem_manager.o && \
    xtensa-lx106-elf-ar -rc libmain.a *.o && \
    rm *.o

RUN xtensa-lx106-elf-objcopy \
      --weaken-symbol Cache_Read_Enable_New \
      --redefine-sym system_restart_local=system_restart_local_sdk \
      /opt/Espressif/ESP8266_NONOS_SDK/lib/libmain.a

RUN mv /opt/Espressif/ESP8266_NONOS_SDK/lib/libc.a /opt/Espressif/ESP8266_NONOS_SDK/lib/libc_sdk.a

# Set up RTOS SDK
RUN git clone -b $DOCKER_TAG https://github.com/cesanta/ESP8266_RTOS_SDK.git /opt/Espressif/ESP8266_RTOS_SDK

RUN for s in malloc free calloc realloc xPortGetFreeHeapSize xPortWantedSizeAlign \
             pvPortMalloc vPortFree pvPortCalloc pvPortZalloc pvPortRealloc; do \
        xtensa-lx106-elf-objcopy --weaken-symbol $s /opt/Espressif/ESP8266_RTOS_SDK/lib/libfreertos.a; \
    done && \
    mv /opt/Espressif/ESP8266_RTOS_SDK/lib/libgcc.a /opt/Espressif/ESP8266_RTOS_SDK/lib/libgcc_sdk.a && \
    xtensa-lx106-elf-objcopy \
      --weaken-symbol Cache_Read_Enable_New \
      /opt/Espressif/ESP8266_RTOS_SDK/lib/libmain.a
# Move aside mbedTLS shipped with the RTOS SDK.
RUN mv /opt/Espressif/ESP8266_RTOS_SDK/lib/libmbedtls.a /opt/Espressif/ESP8266_RTOS_SDK/lib/libmbedtls_sdk.a && \
    mv /opt/Espressif/ESP8266_RTOS_SDK/include/mbedtls /opt/Espressif/ESP8266_RTOS_SDK/include/mbedtls_sdk

ADD cs_lwip /opt/Espressif/cs_lwip
ADD rom.bin rom.elf /opt/Espressif/rom/

ADD mgos_fw_meta.py mklfs mkspiffs mkspiffs8 /usr/local/bin/
ADD serve_core/ /opt/serve_core/
RUN ln -s /opt/serve_core/serve_core.py /usr/local/bin/serve_core.py

ENV MGOS_TARGET_GDB=/opt/Espressif/esp-open-sdk/xtensa-lx106-elf/bin/xtensa-lx106-elf-gdb
ARG DOCKER_TAG
ENV MGOS_SDK_REVISION $DOCKER_TAG
ENV MGOS_SDK_BUILD_IMAGE docker.io/mgos/esp8266-build:$DOCKER_TAG
