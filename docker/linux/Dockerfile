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
        python-is-python3 \
        pkgconf \
        ninja-build \
        git && \
    adduser --disabled-password --gecos "" $WORKFLOW_USER && \
    apt-get purge -y --no-install-recommends --auto-remove adduser && \
    rm -rf /var/lib/apt/lists/* && \
    python -m pip install \
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

HEALTHCHECK CMD [ "sh", "-c", 'if [ ! -x "$VULKAN_SDK/bin/glslc" ]; then echo "glslc missing"; exit 1; fi' ]


# # Install Vulkan SDK.
# ## Copy and extract the vulkan sdk into the image.
# COPY vulkansdk-linux-x86_64-$VULKAN_VERSION.tar.xz .
# RUN mkdir -p ./vulkan && \
#     tar -xf ./vulkansdk-linux-x86_64-$VULKAN_VERSION.tar.xz -C ./vulkan && \
#     rm ./vulkansdk-linux-x86_64-$VULKAN_VERSION.tar.xz

# Strip the Vulkan SDK from unnecessary files to minimize the image size.
# ENV VULKAN_x64_DIR=./vulkan/$VULKAN_VERSION/x86_64
# ENV VULKAN_BIN_DIR=$VULKAN_x64_DIR/bin
# ENV VULKAN_LIB_DIR=$VULKAN_x64_DIR/lib

# RUN cp $VULKAN_BIN_DIR/glslc $VULKAN_x64_DIR && \
#     rm -rf $VULKAN_BIN_DIR/* && \
#     mv $VULKAN_x64_DIR/glslc $VULKAN_BIN_DIR
# RUN rm -f \
#     $VULKAN_LIB_DIR/libVkLayer_*.so \
#     $VULKAN_LIB_DIR/libSPIRV*.so* \
#     $VULKAN_LIB_DIR/libclang*.* \
#     $VULKAN_LIB_DIR/libLLVM*.* \
#     $VULKAN_LIB_DIR/libglslang*.* \
#     $VULKAN_LIB_DIR/libslang*.* \
#     $VULKAN_LIB_DIR/libdx*.* \
#     lib/libdxc*.*

# ENV VULKAN_SDK=$WORKFLOW_HOME/vulkan-app/vulkan/$VULKAN_VERSION/x86_64
# ENV PATH=$VULKAN_SDK/bin:$PATH
# ENV LD_LIBRARY_PATH=$VULKAN_SDK/lib
# ENV VK_LAYER_PATH=$VULKAN_SDK/share/vulkan/explicit_layer.d
# ENV PKG_CONFIG_PATH=$VULKAN_SDK/lib/pkgconfig/
