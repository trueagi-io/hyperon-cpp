# How to build

```
conda create -y -n hyperon python=3.6
conda activate hyperon
conda install -y -c conda-forge cxxtest pybind11 nose

mkdir -p build
cd build
cmake ..
make
make test
```

# Installation

After building the library it can be installed to the system using the commands:
```
cd build
sudo make install
sudo ldconfig
``

The following command can be used to check if the library is installed successfully:
```
python -c "import hyperon"
```

# CircleCI docker

```
cd .circleci
docker build -t trueagi/hyperon-ci .
```
