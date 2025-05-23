FROM ubuntu:24.04

RUN apt-get update && \
    apt-get install -y \
        build-essential \
        gdb \
        cmake \
        make \
        git \
        libboost-all-dev \
        libjsoncpp-dev \
        libopus-dev \
        libssl-dev \
        libvpx-dev \
        libupnp-dev \
        uuid-dev \
        libasound2-dev \
        libavdevice-dev \
        libavformat-dev \
        libavcodec-dev \
        libswscale-dev \
        libavutil-dev \
        libv4l-dev \
        libopencore-amrnb-dev \
        libopencore-amrwb-dev

RUN git clone https://github.com/pjsip/pjproject.git --depth=1 /opt/pjproject

WORKDIR /opt/pjproject

RUN ./configure && \
    make dep && \
    make -j$(nproc) && \
    make install && \
    ldconfig

WORKDIR /client

COPY . /client/

RUN cd /client && rm -rf build && \
    cmake -B build && \
    cmake --build build -j$(nproc)

CMD [ "client/build/voip/voip" ]
