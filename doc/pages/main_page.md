# Introduction {#mainpage}

This is the documentation for the SystemC-based simulation of the ALPIDE and ITS. The simulation model aims to be a fairly accurate model of the ALPIDE chip's readout chain, but with the performance benefits that C++ and SystemC offer compared to traditional HDL simulations.


## Building the code and getting started

Instructions from the README on how to build, use and run the code can be found here:

[README.md]:(@ref readme)

## Coding style

Below is a brief description of the coding style used for this code.

### Class member names

- SystemC port and signal names: all lowercase, with s_ prefix, and _in or _out postfix to indicate input/output. They should also be at the top of the class definition, separated from normal C++ member variables.
- Some SystemC ports/signals have names ending in _debug. These signals are not used for anything, they are just "probes" that allow some signals and quantities to appear in the waveform .vcd files (when they normally would not, such as a fifo size). Note that these _debug signals are typically delayed by a clock cycle compared to the signal or quantity represent.
- Class names: Upper camel case names
- Class methods: Lower camel case names (except for constructor etc. obviously)
- Class member variables: Lower camel case names, starting with an m to indicate member variable, e.g. mNumEvents.
- Class function parameters: All lowercase, words separated with underscore, to separate from class member variables. Example: num_events.

### Documentation

Doxygen style comments
