FROM ubuntu:22.10

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/PDT 
RUN apt-get update &&  apt-get install -y \
    git make g++ libpng-dev cmake libspdlog-dev qttools5-dev

# install nfpm
RUN echo 'deb [trusted=yes] https://repo.goreleaser.com/apt/ /' | tee /etc/apt/sources.list.d/goreleaser.list && \
   apt-get update && apt-get install -y nfpm

COPY . /mcmap
