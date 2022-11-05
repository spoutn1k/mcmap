FROM ubuntu:22.10

RUN apt-get update && apt-get install -y \
    git make g++ libpng-dev cmake libspdlog-dev sudo

# install nfpm
RUN echo 'deb [trusted=yes] https://repo.goreleaser.com/apt/ /' | tee /etc/apt/sources.list.d/goreleaser.list && \
   apt-get update && apt-get install -y nfpm

COPY . /app
