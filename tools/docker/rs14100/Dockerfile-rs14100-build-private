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

RUN DEBIAN_FRONTEND=noninteractive add-apt-repository -y -u ppa:team-gcc-arm-embedded/ppa && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y -q gcc-arm-embedded && \
    apt-get clean

ARG RS14100_SDK
ADD tmp/$RS14100_SDK /opt/$RS14100_SDK

ADD mgos_fw_meta.py mklfs mkspiffs mkspiffs8 /usr/local/bin/
ADD serve_core/ /opt/serve_core/
RUN ln -s /opt/serve_core/serve_core.py /usr/local/bin/serve_core.py

ARG DOCKER_TAG
ENV MGOS_TARGET_GDB /usr/bin/arm-none-eabi-gdb
ENV MGOS_SDK_REVISION $DOCKER_TAG
ENV MGOS_SDK_BUILD_IMAGE docker.cesanta.com/rs14100-build-private:$DOCKER_TAG
ENV RS14100_SDK_PATH /opt/$RS14100_SDK
