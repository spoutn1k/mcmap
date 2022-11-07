COMMIT_SHA_SHORT ?= $(shell git rev-parse --short=12 HEAD)
PWD_DIR:= ${CURDIR}
SHELL := /bin/bash

default: help;

# ======================================================================================
build-builder-debian: ## build the builder image that contains the source code
	@docker build -f build-debian.dockerfile -t mcmap-builder-debian:latest ./..

build-debian: build-builder-debian ## build for linux using docker
	@mkdir -p out
	@docker run -it --rm -v ${PWD_DIR}/out:/out mcmap-builder-debian:latest /bin/bash -c "mkdir -p /mcmap/build && \
		cd /mcmap/build && \
		cmake .. && \
		make -j mcmap mcmap-gui && \
		cp /mcmap/build/bin/* /out"

package-debian: build-debian ## package the just compiled binary
	@docker run -it --rm -v ${PWD_DIR}/out:/out mcmap-builder-debian:latest /bin/bash -c "cd /out && \
		nfpm pkg -f /mcmap/package-debian/nfpm.yaml -p deb"


help: ## Show this help
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST)  | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m- %-20s\033[0m %s\n", $$1, $$2}'
