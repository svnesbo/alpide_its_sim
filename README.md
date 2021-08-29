# Getting started - Building and running the code {#readme}

A SystemC model the Alpide chip with simulations for ITS, FoCal, and the UiB pCT.

## Building the simulation program

Building the project requires GCC with C++11 support (included from version 4.8), and CMake is used for building.

### Required Libraries
- Requires Qt5, boost libraries, and obviously SystemC. On ubuntu, they can be installed with:

- Qt5:
```
sudo apt-get install qt5-default
```

-  boost (full installation):
```
sudo apt-get install libboost-all-dev
```

- SystemC:

Can be downloaded from http://accellera.org/downloads/standards/systemc

The code has been tested and developed with version 2.3.1 of SystemC. Refer to SystemC documentation for build/install instructions.


### Compiling the code

```
mkdir build
cd build
cmake ..
make
```

### To compile documentation
Requires doxygen. From the build directory:

```
make doc
```


### Running unit tests
3 tests will be compiled in the build directory:
* alpide_test - Tests the whole Alpide SystemC with random input data, parses output from the model and verifies that all the correct pixel hits were sent out
* pixel_matrix_test - Test of PixelMatrix class
* pixel_col_test - Test of PixelDoubleColumn class - verifies that pixel priority encoder has the desired prioritization, etc.

To compile and run the unit tests:

```
make check
```

## Running the simulation:

The program expects to find settings files etc in <current working directory>/config, and should preferably be run from the simulation project's top directory:

```
bin/alpide_its_sim
```

The program requires a settings.txt file with simulation settings. If it does not exist, running the program generates a settings.txt file with default settings. This file can be edited and the simulation rerun to use those settings. The settings.txt file will not be overwritten.

Simulation results will be saved in sim_output/Run {timestamp}/


## To process simulation data:

There are some root macros, python scripts and jupyter notebooks to analyze simulated data. Most of them are quite messy and poorly written :/

To compile the root software:
```
mkdir -p analysis/build
cd analysis/build
cmake ..
make
```

Use the `process_readout_trigger_stats.C` program to analyze simulation data, generate a root file and some plots.

Run:
```
process_readout_trigger_stats --help
```
for instructions on how to use it. It should work ok for ITS simulations, but quite possibly not for pCT and FoCal.
