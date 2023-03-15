FROM --platform=linux/amd64 ubuntu:20.04 AS build
# FROM --platform=linux/amd64 eclipse-mosquitto:2.0.15

ENV VERSION=2.0.15
# Install necessary build tools and libraries
RUN DEBIAN_FRONTEND=noninteractive apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install --no-install-recommends -y \
    build-essential \
    vim \
    curl \
    wget \
    git \
    cmake \
    libssl-dev \
    libwebsockets-dev \
    libsqlite3-dev \
    uuid-dev \
    libcjson-dev \
    libcunit1 \
    xsltproc \
    libxml2 \
    docbook-xsl

RUN DEBIAN_FRONTEND=noninteractive apt-get install -y --reinstall ca-certificates && \
    mkdir -p /usr/local/share/ca-certificates/cacert.org && \
    wget -P /usr/local/share/ca-certificates/cacert.org http://www.cacert.org/certs/root.crt http://www.cacert.org/certs/class3.crt && \
    update-ca-certificates

# Clone the Mosquitto repository
# server certificate verification failed. CAfile: none CRLfile: none
RUN git clone --depth=1 https://github.com/eclipse/mosquitto.git && \
    cd mosquitto && \
    git remote set-branches origin develop && \
    git fetch --depth 1 origin develop && \
    #git checkout develop
    git fetch --depth 1 origin 6f574f80ea151a328fd94789c2b336e0bd1fa115 && \
    git checkout 6f574f80ea151a328fd94789c2b336e0bd1fa115

# RUN curl -OL https://github.com/eclipse/mosquitto/archive/refs/tags/v${VERSION}.tar.gz && \
#     tar -xzf v${VERSION}.tar.gz && \
#     rm v${VERSION}.tar.gz && \
#     mv mosquitto-${VERSION} mosquitto

# Compile and install Mosquitto
WORKDIR /mosquitto
RUN make WITH_DOCS=no && \
    make install

# Set the working directory for the plugin

RUN mkdir -p /mosquitto/plugins/mosquitto_topic_jail_all
WORKDIR /mosquitto/plugins/mosquitto_topic_jail_all

# Copy the plugin source files to the Docker container
COPY . .

# Compile the plugin
RUN make

FROM --platform=linux/amd64 debian:bullseye-slim

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    ca-certificates \
    libcjson1

WORKDIR /app

COPY --from=build /mosquitto/client/mosquitto_pub /usr/bin/mosquitto_pub
COPY --from=build /mosquitto/client/mosquitto_sub /usr/bin/mosquitto_sub
COPY --from=build /mosquitto/lib/libmosquitto.so.1 /usr/lib/libmosquitto.so.1
COPY --from=build /mosquitto/src/mosquitto /usr/bin/mosquitto
COPY --from=build /mosquitto/apps/mosquitto_passwd/mosquitto_passwd  /usr/bin/mosquitto_passwd
COPY --from=build /mosquitto/plugins/mosquitto_topic_jail_all/mosquitto_topic_jail_all.so /app/mosquitto_topic_jail_all.so
COPY --from=build /mosquitto/plugins/mosquitto_topic_jail_all/test.conf /app/test.conf
COPY --from=build /mosquitto/plugins/mosquitto_topic_jail_all/test.sh /app/test.sh
RUN useradd -ms /bin/bash  mosquitto
USER mosquitto

CMD ["/bin/bash"]