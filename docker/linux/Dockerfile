FROM debian:trixie
LABEL author="Alexander Olsson"
LABEL description="Dockerfile for preparing a Vulkan image used in CI/CD."

# Set environment variables.
ENV DEBIAN_FRONTEND=noninteractive
ENV WORKFLOW_USER=runner
ENV VULKAN_VERSION=1.4.321.1

# Update and install packages.
RUN apt update && apt install -y \
    vulkan-tools \
    libvulkan-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy in the source code.
# COPY src ./src
# EXPOSE 8080

# Create a non-root user and working directory.
RUN useradd -m -u 1001 $WORKFLOW_USER \
    && mkdir -p /home/$WORKFLOW_USER/vulkan-app \
    && chown -R $WORKFLOW_USER /home/$WORKFLOW_USER/vulkan-app

USER $WORKFLOW_USER
WORKDIR /home/$WORKFLOW_USER/vulkan-app

# Install Vulkan SDK

# TODO: Fix local paths, copy tar into docker folder? How to manage GIT, maybe git LFS?
# Copy and extract the vulkan sdk into the image.
COPY vulkansdk-linux-x86_64-$VULKAN_VERSION.tar.xz .
RUN mkdir -p ~/vulkan && \
    tar -xf ./vulkansdk-linux-x86_64-$VULKAN_VERSION.tar.xz -C ./vulkan && \
    rm ./vulkansdk-linux-x86_64-$VULKAN_VERSION.tar.xz

ENV VULKAN_SDK=/home/$WORKFLOW_USER/vulkan-app/vulkan/$VULKAN_VERSION/x86_64
ENV PATH=$VULKAN_SDK/bin:$PATH
ENV LD_LIBRARY_PATH=$VULKAN_SDK/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
ENV VK_LAYER_PATH=$VULKAN_SDK/share/vulkan/explicit_layer.d
ENV PKG_CONFIG_PATH=$VULKAN_SDK/lib/pkgconfig/

CMD ["bash"]
