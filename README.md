## Building and Running the code {#building_and_running}

A simple Dataflow SystemC Model of ITS and the Alpide chip

### Getting started - building the project

Building the project requires GCC with C++11 support (included from version 4.8.x I think). 

#### Required Libraries
- Requires qmake and boost libraries. On ubuntu, they can be installed with:

- qmake:
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


#### Environment

The project's makefile expects to find the path to the base directory of the SystemC installation in $SYSTEMC_HOME.
It may also be necessary to add the path to the SystemC libraries to $LD_LIBRARY_PATH. Adding something like this to $HOME/.profile should do (for version 2.3.1):
```
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/path/to/systemc-2.3.1/lib-linux64/
export SYSTEMC_HOME=/path/to/systemc-2.3.1
```



#### Compiling the code
```
cd alpide_dataflow_model
make
```

### To run:
```
./alpide_dataflow_model
```


The program requires a settings.txt file with simulation settings. If it does not exist, running the program generates a settings.txt file with default settings. This file can be edited and the simulation rerun to use those settings. The settings.txt file will not be overwritten.

Simulation results will be saved in sim_output/Run {timestamp}/


Simulation output is stored in alpide_dataflow_model/sim_output/{timestamp}/

### To process simulation data:
```
cd alpide_dataflow_model/sim_output/{timestamp}/
root -b -q -l '../../process/process_event_data.C+("physics_events_data.csv")'
```

