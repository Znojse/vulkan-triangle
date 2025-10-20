FROM debian:trixie-slim AS builder

# Set environment variables.
ENV VULKAN_VERSION=1.4.321.1

WORKDIR /tmp/vulkan

# Copy and extract the Vulkan SDK and keep only required
# includes/libraries/binaries for a minimal CI/CD Vulkan SDK image.
COPY vulkansdk-linux-x86_64-$VULKAN_VERSION.tar.xz .
RUN apt-get update && apt-get install -y xz-utils && \
    tar -xf ./vulkansdk-linux-x86_64-$VULKAN_VERSION.tar.xz && \
    mkdir -p /minimal-sdk/bin /minimal-sdk/lib /minimal-sdk/include && \
    cp    ./$VULKAN_VERSION/x86_64/bin/glslc                 /minimal-sdk/bin && \
    cp -r ./$VULKAN_VERSION/x86_64/include                   /minimal-sdk     && \
    cp    ./$VULKAN_VERSION/x86_64/lib/libshaderc_shared.so* /minimal-sdk/lib && \
    cp    ./$VULKAN_VERSION/x86_64/lib/libvulkan.so*         /minimal-sdk/lib && \
    rm -rf vulkansdk-linux-x86_64-$VULKAN_VERSION.tar.xz ./$VULKAN_VERSION


# Stage 2: Final Vulkan image.
FROM debian:trixie-slim
LABEL description="Dockerfile for preparing a Vulkan image used in CI/CD."

# Set environment variables.
ENV DEBIAN_FRONTEND=noninteractive
ENV WORKFLOW_USER=runner
ENV WORKFLOW_HOME=/home/$WORKFLOW_USER
ENV CONAN_HOME=$WORKFLOW_HOME/.conan2

# Update and install packages as well as create a non-root user.
RUN apt-get update && apt-get install -y --no-install-recommends \
        adduser \
        python3 \
        python3-pip \
        pkgconf \
        build-essential \
        clang-tidy \
        ninja-build \
        libgl-dev \
        libgl1-mesa-dev \
        libx11-dev \
        libx11-xcb-dev \
        libfontenc-dev \
        libice-dev \
        libsm-dev \
        libxau-dev \
        libxaw7-dev \
        libxcomposite-dev \
        libxcursor-dev \
        libxdamage-dev \
        libxfixes-dev \
        libxi-dev \
        libxinerama-dev \
        libxkbfile-dev \
        libxmuu-dev \
        libxrandr-dev \
        libxrender-dev \
        libxres-dev \
        libxss-dev \
        libxtst-dev \
        libxv-dev \
        libxxf86vm-dev \
        libxcb-glx0-dev \
        libxcb-render0-dev \
        libxcb-render-util0-dev \
        libxcb-xkb-dev \
        libxcb-icccm4-dev \
        libxcb-image0-dev \
        libxcb-keysyms1-dev \
        libxcb-randr0-dev \
        libxcb-shape0-dev \
        libxcb-sync-dev \
        libxcb-xfixes0-dev \
        libxcb-xinerama0-dev \
        libxcb-cursor-dev \
        libxcb-dri2-0-dev \
        libxcb-dri3-dev \
        libxcb-present-dev \
        libxcb-composite0-dev \
        libxcb-ewmh-dev \
        libxcb-res0-dev \
        libxcb-util-dev \
        libxcb-util0-dev \
        git && \
    adduser --disabled-password --gecos "" $WORKFLOW_USER && \
    apt-get purge -y --no-install-recommends --auto-remove adduser && \
    apt-get clean && rm -rf /var/lib/apt/lists/* && \
    python3 -m pip install \
        conan==2.19.1 \
        cmake>=3.29 \
        --no-cache-dir --break-system-packages && \
    rm -rf /root/.cache/pip && \
    mkdir -p $WORKFLOW_HOME && \
    chown -R $WORKFLOW_USER:$WORKFLOW_USER $WORKFLOW_HOME

# Change to a non-root user and set working directory.
USER $WORKFLOW_USER
WORKDIR $WORKFLOW_HOME/vulkan-app

# Copy only minimal Vulkan SDK.
COPY --chown=$WORKFLOW_USER --from=builder /minimal-sdk $WORKFLOW_HOME/vulkan-sdk

# Set the Vulkan SDK environment variables.
ENV VULKAN_SDK=$WORKFLOW_HOME/vulkan-sdk
ENV PATH=$VULKAN_SDK/bin:$PATH
ENV LD_LIBRARY_PATH=$VULKAN_SDK/lib
ENV PKG_CONFIG_PATH=$VULKAN_SDK/lib/pkgconfig/

HEALTHCHECK CMD sh -c '\
  GLSLC=$VULKAN_SDK/bin/glslc && \
  test -x "$GLSLC" || { echo "Missing glslc"; exit 1; } && \
  test -f "$VULKAN_SDK/lib/libvulkan.so" || { echo "Missing Vulkan lib"; exit 1; } && \
  which cmake >/dev/null 2>&1 || { echo "cmake not found"; exit 1; } && \
  which conan >/dev/null 2>&1 || { echo "conan not found"; exit 1; }'
