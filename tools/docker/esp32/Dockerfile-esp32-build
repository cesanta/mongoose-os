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

RUN DEBIAN_FRONTEND=noninteractive apt-get install -y -q \
      python-cryptography python3-cryptography \
      python-future python3-future \
      python-pkg-resources python3-pkg-resources \
      python-setuptools python3-setuptools \
      rsync && \
    apt-get clean

RUN useradd -d /opt/Espressif -m -s /bin/bash user && chown -R user /opt

ARG TOOLCHAIN_VERSION
RUN cd /opt/Espressif && wget -q https://dl.espressif.com/dl/xtensa-esp32-elf-linux64-$TOOLCHAIN_VERSION.tar.gz && \
    tar xzf xtensa-esp32-elf-linux64-$TOOLCHAIN_VERSION.tar.gz && \
    rm xtensa-esp32-elf-linux64-$TOOLCHAIN_VERSION.tar.gz
ENV PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/opt/Espressif/xtensa-esp32-elf/bin
ADD ct_path.sh /etc/profile.d

ARG DOCKER_TAG
RUN git clone --recursive --branch $DOCKER_TAG https://github.com/cesanta/esp-idf /opt/Espressif/esp-idf
RUN cd /opt/Espressif/esp-idf && git tag v$DOCKER_TAG
ENV IDF_PATH=/opt/Espressif/esp-idf

# Pre-build configuration tools
RUN cd $IDF_PATH/tools/kconfig && make conf-idf mconf-idf

ADD rom.bin rom.elf /opt/Espressif/rom/

ADD mgos_fw_meta.py mklfs mkspiffs mkspiffs8 /usr/local/bin/
ADD serve_core/ /opt/serve_core/
RUN ln -s /opt/serve_core/serve_core.py /usr/local/bin/serve_core.py

ARG DOCKER_TAG
ENV MGOS_TARGET_GDB /opt/Espressif/xtensa-esp32-elf/bin/xtensa-esp32-elf-gdb
ENV MGOS_SDK_REVISION $DOCKER_TAG
ENV MGOS_SDK_BUILD_IMAGE docker.io/mgos/esp32-build:$DOCKER_TAG
