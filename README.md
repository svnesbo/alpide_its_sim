A simple Dataflow SystemC Model of ITS and the Alpide chip


Building:

- Requires qmake and boost libraries. On ubuntu, they can be installed with:

- qmake:
```
sudo apt-get install qt5-default
```

-  boost (full installation):
```
sudo apt-get install libboost-all-dev
```

To build:
```
cd alpide_dataflow_model
make
```

To run:
```
./alpide_dataflow_model
```


The program requires a settings.txt file with simulation settings. If it does not exist, running the program generates a settings.txt file with default settings. This file can be edited and the simulation rerun to use those settings. The settings.txt file will not be overwritten.

Simulation results will be saved in sim_output/Run <timestamp>/


Simulation output is stored in alpide_dataflow_model/sim_output/<timestamp>/

- To process simulation data:
```
cd alpide_dataflow_model/sim_output/<timestamp>/
root -b -q -l '../../process/process_event_data.C+("physics_events_data.csv")'
```

