FROM ubuntu:22.04
# Install the xz-utils package
RUN DEBIAN_FRONTEND=noninteractive apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential \
    libc-ares-dev \
    libwebsockets-dev \
    libssl-dev \
    git \
    curl \
    wget \
    vim \
    gnupg \
    linux-headers-generic \
    libcjson-dev \
    libsqlite3-dev \
    xsltproc \
    libcunit1 \
    libcunit1-dev \
    pkg-config \
    gdb


COPY ./reinstall-cmake.sh /tmp/

RUN chmod +x /tmp/reinstall-cmake.sh && /tmp/reinstall-cmake.sh
