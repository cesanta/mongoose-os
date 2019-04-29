DOCKERFILES = ubuntu-build
DOCKER_FLAGS = --build-arg=DOCKER_TAG=$(DOCKER_TAG) 

include ../docker.mk

DISTR_PATH = ../distr

clean:
	rm -rf mgos_fw_meta.py mklfs mkspiffs mkspiffs8
