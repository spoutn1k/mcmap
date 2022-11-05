COMMIT_SHA_SHORT ?= $(shell git rev-parse --short=12 HEAD)
PWD_DIR:= ${CURDIR}
SHELL := /bin/bash

default: help;

# ======================================================================================
build-builder: ## build the builder image that contains the source code
	@docker build -f build-linux.dockerfile -t mcmap-builder:latest ./..

build-linux: build-builder ## build for linux using docker
	@mkdir -p out
	@docker run -it -v ${PWD_DIR}/out:/out mcmap-builder:latest /bin/bash -c "cd /app && \
		mkdir -p /app/build && cd /app/build && \
		cmake .. && \
		make -j && \
		chmod -R 777 /app/build/bin/* && \
		cp -R /app/build/bin/* /out"

package-linux: build-linux ## package the just compiled binary
	@docker run -it -v ${PWD_DIR}/out:/out mcmap-builder:latest /bin/bash -c "cd /out && \
		pwd && ls -al && \
		nfpm pkg -f /app/linux-build/nfpm.yaml -p deb"


help: ## Show this help
	@egrep '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST)  | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m·%-20s\033[0m %s\n", $$1, $$2}'