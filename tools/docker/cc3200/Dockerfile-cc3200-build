FROM ubuntu:xenial

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

# Note: CC3200 bootloader doesn't build properly with newer GCC.
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y -q \
      gcc-arm-none-eabi gdb-arm-none-eabi libnewlib-arm-none-eabi libnewlib-dev && \
    apt-get clean

ARG TI_COMPILER_DIR
ADD tmp/$TI_COMPILER_DIR /opt/$TI_COMPILER_DIR

# Create our own version of libc - with no HOSTIO and malloc functions.
RUN cd /opt/$TI_COMPILER_DIR/lib && rm *.lib && \
    ./mklib --index=libc.a --pattern=rtsv7M4_T_le_eabi.lib \
            --extra_options='-O4 --opt_for_speed=0' \
            --compiler_bin_dir=../bin \
            --name=rtsv7M4_T_le_eabi_cesanta.lib --install_to=. \
            --parallel=4

ARG SDK_DIR
ADD tmp/$SDK_DIR /opt/$SDK_DIR

ARG NWP_SP_FILE
ADD tmp/$NWP_SP_FILE tmp/$NWP_SP_FILE.sign /opt/

ADD mgos_fw_meta.py mklfs mkspiffs mkspiffs8 /usr/local/bin/
ADD serve_core/ /opt/serve_core/
RUN ln -s /opt/serve_core/serve_core.py /usr/local/bin/serve_core.py

ARG DOCKER_TAG
ENV SDK_PATH=/opt/$SDK_DIR/cc3200-sdk
ENV TI_COMPILER_PATH=/opt/$TI_COMPILER_DIR
ENV CC3200_SP_FILE=/opt/$NWP_SP_FILE
ENV MGOS_TARGET_GDB=/usr/bin/arm-none-eabi-gdb
ENV MGOS_SDK_REVISION $DOCKER_TAG
ENV MGOS_SDK_BUILD_IMAGE docker.io/mgos/cc3200-build:$DOCKER_TAG
