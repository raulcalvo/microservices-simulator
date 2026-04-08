FROM ubuntu:22.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies via APT for a much faster build than vcpkg
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libzmq3-dev \
    libspdlog-dev \
    nlohmann-json3-dev \
    libcxxopts-dev \
    libfmt-dev

WORKDIR /app
COPY . .

# Configure and build
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build -j$(nproc)

# Final smaller image
FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    libzmq5 \
    libspdlog1 \
    libfmt8 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/micro_node /app/micro_node
