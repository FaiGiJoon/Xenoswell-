FROM --platform=linux/amd64 debian:bookworm-slim
RUN apt-get update && apt-get install -y --no-install-recommends cmake ninja-build unzip curl ca-certificates make
ENV V=v0.24
RUN mkdir -p /opt/gcc /opt/sdk && cd /tmp \
 && curl -fsSLO https://github.com/Lameguy64/PSn00bSDK/releases/download/$V/gcc-mipsel-none-elf-12.3.0-linux.zip \
 && curl -fsSLO https://github.com/Lameguy64/PSn00bSDK/releases/download/$V/PSn00bSDK-0.24-Linux.zip \
 && unzip -q gcc-*.zip -d /opt/gcc && unzip -q PSn00bSDK-*.zip -d /opt/sdk && rm *.zip
RUN ln -s /opt/gcc /opt/gcc-root; ln -s /opt/sdk/PSn00bSDK-0.24-Linux /opt/psn00b
ENV PATH=/opt/gcc-root/bin:/opt/psn00b/bin:$PATH PSN00BSDK_LIBS=/opt/psn00b/lib/libpsn00b
WORKDIR /src
