DOCKERFILES = cc3200-build
SPIFFS_CONFIG_PATH = /vfs-fs-spiffs/include/cc3200
DOCKER_FLAGS = --build-arg=TI_COMPILER_DIR=$(TI_COMPILER_DIR) \
               --build-arg=NWP_SP_FILE=$(NWP_SP_FILE) \
               --build-arg=SDK_DIR=$(SDK_DIR) \
               --build-arg=DOCKER_TAG=$(DOCKER_TAG)

# SDK cannot be downloaded directly (requires registering on the TI site).
# Furthermore, it is distributed as a Windows binary only (boo). So:
# 1) Go to http://www.ti.com/tool/CC3200SDK and download the SDK exe file.
# 2) "Install" the SDK (which just extracts a bunch of files).
#    You don't need Windows for this, Wine will do just fine.
# 3) The default install path is C:\TI\CC3200SDK_?.?.?
#    Take that entire directory and put here.
SDK_DIR = CC3200SDK_$(firstword $(subst -, ,$(DOCKER_TAG)))

# Get CCS and find it in $TI_DIR/ccsv*/tools/compiler/ti-cgt-arm_?.?.?
TI_COMPILER_DIR = ti-cgt-arm_16.9.9.LTS

# NWP service pack.
# These are from $TI_DIR/CC3100_CC3200_ServicePack_X.X.X.X-X.X.X.X/ota
# (names changed slightly).
NWP_SP_FILE = ota_1.0.1.11-2.8.0.0.ucf

include ../docker.mk

$(TI_COMPILER_DIR) $(SDK_DIR) $(NWP_SP_FILE) $(NWP_SP_FILE).sign:
	@echo ERROR: You must have $@ in the current directory to build this image
	@exit 1

docker-pre-build-cc3200-build: $(TI_COMPILER_DIR) $(SDK_DIR) $(NWP_SP_FILE) $(NWP_SP_FILE).sign \
                               fw_meta.py serve_core.py mklfs mkspiffs mkspiffs8

clean:
	rm -rf fw_meta.py serve_core.py mklfs mkspiffs mkspiffs8
