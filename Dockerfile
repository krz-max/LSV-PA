FROM ubuntu:22.04

# Avoid prompts from apt
ENV DEBIAN_FRONTEND=noninteractive

# Update and install necessary packages
RUN apt-get update && apt-get install -y \
    build-essential \
    libreadline-dev \
    # graphviz \
    unzip \
    curl \
    wget \
    # gv \
    git-all \
    git-lfs \
    && rm -rf /var/lib/apt/lists/*


# RUN wget https://github.com/ArtifexSoftware/ghostpdl-downloads/releases/download/gs10031/ghostscript-10.03.1.tar.gz \
#     && tar -xzf ghostscript-10.03.1.tar.gz \
#     && cd ghostscript-10.03.1 \
#     && ./configure \
#     && make \
#     && make install \
#     && cd .. \
#     && rm -rf ghostscript-10.03.1.tar.gz 

# Set the working directory in the container
WORKDIR /workspace

# Command to run when starting the container
CMD ["/bin/bash"]
