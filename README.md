# Getting started - Building and running the code {#readme}

A simple Dataflow SystemC Model of ITS and the Alpide chip. This repo includes the SystemC Alpide module located in source/bench/Alpide, and a standalone simulation program for the Alpide module located in software/alpide_dataflow_sim.

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
mkdir software/alpide_dataflow_sim/build
cd software/alpide_dataflow_sim/build
cmake ..
make
```

### To compile documentation
Requires doxygen. From the build directory:

```
make doc
```

## Running the code:

The program expects to find settings files etc in <current working directory>/config, and should preferably be run from the simulation project's top directory:

```
cd software/alpide_dataflow_sim/
bin/alpide_dataflow_model
```

The program requires a settings.txt file with simulation settings. If it does not exist, running the program generates a settings.txt file with default settings. This file can be edited and the simulation rerun to use those settings. The settings.txt file will not be overwritten.

Simulation results will be saved in sim_output/Run {timestamp}/


## To process simulation data:

Analysis of simulation data is currently limited to a simple script which plots distributions of multiplicity and event rates for the events that were generated during the simulation.

```
cd sim_output/Run {timestamp}/
root -b -q -l '../../analysis/process_event_data.C+("physics_events_data.csv")'
```
