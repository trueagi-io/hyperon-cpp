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
