FROM ubuntu:22.04 AS builder

# ============================================================
# --- Base environment ---
# ============================================================
FROM ubuntu:22.04 AS base
RUN apt-get update && apt-get install -y \
    git cmake g++ make wget curl unzip zlib1g-dev \
    libcurl4-openssl-dev libeigen3-dev && \
    rm -rf /var/lib/apt/lists/*

# --- Remove any system Abseil (avoid ABI conflicts) ---
RUN apt-get purge -y 'libabsl*' || true

# --- Make /usr/local/lib discoverable ---
RUN echo "/usr/local/lib" > /etc/ld.so.conf.d/usr-local.conf && ldconfig


# ============================================================
# --- Build & install Abseil (cached) ---
# ============================================================
FROM base AS abseil
ARG ABSL_VERSION=20240722.0
RUN git clone --branch ${ABSL_VERSION} --depth 1 https://github.com/abseil/abseil-cpp.git /tmp/abseil && \
    cd /tmp/abseil && cmake -S . -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DCMAKE_INSTALL_PREFIX=/usr/local && \
    cmake --build build -j$(nproc) && cmake --install build && rm -rf /tmp/abseil


# ============================================================
# --- Build & install Protobuf (self-contained) ---
# ============================================================
FROM abseil AS protobuf
ARG PROTOBUF_VERSION=v31.1
RUN git clone --branch ${PROTOBUF_VERSION} --depth 1 https://github.com/protocolbuffers/protobuf.git /tmp/protobuf && \
    cd /tmp/protobuf && \
    cmake -S . -B build \
        -Dprotobuf_BUILD_TESTS=OFF \
        -Dprotobuf_BUILD_SHARED_LIBS=ON \
        -Dprotobuf_USE_EXTERNAL_GTEST=OFF \
        -Dprotobuf_ABSL_PROVIDER=module \
        -Dprotobuf_WITH_ZLIB=ON \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local && \
    cmake --build build -j$(nproc) && \
    cmake --install build && \
    ldconfig && \
    ls -l /usr/local/lib/libprotobuf* && \
    ls -l /usr/local/lib/libabsl_* && \
    rm -rf /tmp/protobuf


# ============================================================
# --- Intel Decimal Math Library (libbid.a) ---
# ============================================================
FROM protobuf AS libbid
RUN cd /tmp && \
    wget http://www.netlib.org/misc/intel/IntelRDFPMathLib20U2.tar.gz && \
    tar -xzf IntelRDFPMathLib20U2.tar.gz && \
    cd IntelRDFPMathLib20U2/LIBRARY && \
    make CC=gcc CALL_BY_REF=0 GLOBAL_RND=0 GLOBAL_FLAGS=0 UNCHANGED_BINARY_FLAGS=0 && \
    cp libbid.a /usr/local/lib/ && \
    mkdir -p /usr/local/include/libbid && \
    cp -r ../LIBRARY/src /usr/local/include/libbid/ && \
    ldconfig && \
    rm -rf /tmp/IntelRDFPMathLib20U2*


# ============================================================
# --- Project build stage ---
# ============================================================
FROM libbid AS build-stage
WORKDIR /app

# Copy dependency files first for caching
COPY CMakeLists.txt ./
COPY cmake/ ./cmake/
COPY external/ ./external/

# Preconfigure CPM and dependency fetching
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release || true

# Copy the rest of the project
COPY . .

RUN git submodule update --init --recursive
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc)


# ============================================================
# --- Runtime stage (final image) ---
# ============================================================
FROM ubuntu:22.04 AS runtime
COPY --from=build-stage /usr/local /usr/local
COPY --from=build-stage /app/build /app/build
WORKDIR /app/build
EXPOSE 4002
CMD ["./IBKR_get_market_data"]
