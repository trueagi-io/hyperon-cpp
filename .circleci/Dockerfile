FROM cimg/base:2021.01

# install conda
ENV MINICONDA_VER=py38_4.9.2
RUN wget https://repo.anaconda.com/miniconda/Miniconda3-${MINICONDA_VER}-Linux-x86_64.sh
RUN chmod +x Miniconda3-${MINICONDA_VER}-Linux-x86_64.sh
RUN ./Miniconda3-${MINICONDA_VER}-Linux-x86_64.sh -b
RUN rm ./Miniconda3-${MINICONDA_VER}-Linux-x86_64.sh
RUN eval "$(~/miniconda3/bin/conda shell.bash hook)" && conda init

# setup conda env
ENV PATH=~/miniconda3/bin:$PATH
RUN conda create -y -n hyperon python=3.6
SHELL ["/home/circleci/miniconda3/bin/conda", "run", "-n", "hyperon", "/bin/bash", "-c"]
RUN conda install -y -c conda-forge pybind11 nose

# install software
RUN sudo apt-get update && \
    sudo apt-get install --yes \
    cmake \
    cxxtest \
    && sudo rm -rf /var/lib/apt/lists/*

RUN rm -rf .empty
