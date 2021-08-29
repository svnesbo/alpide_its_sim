# Alpide Model Description

The top-level Alpide class inherits from the PixelMatrix class, and it has an instance of the TopReadoutUnit (TRU) class and 32 instances of the RegionReadoutUnit (RRU) class.

At the top-level, and in the TRU and RRU classes, the logic is implemented using a combination of SystemC and regular C++ functions, but mainly SystemC. But the PixelMatrix class and its members are implemented purely in C++.

## Alpide
The Alpide top-level object has an interface that is a combination of SystemC ports and regular C++ functions. It provides a relatively convenient and minimalistic interface for the user, only a few SystemC signals needs to be defined, and setting pixels in the class is done with some pretty straightforward C++ functions.

### SystemC ports that must be connected:

- s_system_clk_in	- connect a 40MHz clock to this port
- s_strobe_n_in - falling edge sensitive strobe input
- s_chip_ready_out - output that indicates that the chip is ready to receive pixel hits
- s_serial_data_out - data output from the Alpide, 3 x 8-bit words (not really a "serial" output, but it represents one)

### C++ member functions
The only C++ functions that the user really needs to be concerned with is the constructor, and the setPixel() function. As an alternative to calling setPixel() directly, one can make a TriggerEvent object that contains all the pixel hits, and use TriggerEvent::feedHitsToChip() to set the pixels.

### SystemC Methods/Processes in the Alpide class
There is one primary "process" in the Alpide class, the mainProcess(), which is used as a SystemC SC_METHOD and is sensitive to the rising edge of the sytem clock.

The mainProcess() in turn calls the following functions:
- strobeInput()
- frameReadout()
- dataTransmission()
- updateBusyStatus()

Together these functions are responsible for the SystemC part of the implementation of the top-level Alpide class.

#### Strobe Input
The strobeInput() function waits for the strobe to be asserted or deasserted.
When the strobe is assserted, it tries to allocate a Multi Event Buffer (MEB) for the next event. Depending on how many MEBs are left, and whether the chip is in triggered on continuous mode, the chip may or may not be able to reserve a new MEB slice, in the latter case we would get a busy violation.
When the strobe is deasserted this function will push the frame start word for this event frame to the frame start FIFO.

### Frame Readout
When there are new events in the Multi Event Buffers, an FSM in this function is responsible for starting readout from the MEB. Fully read out MEB slices are cleared and the event's frame end word is pushed to the frame end FIFO in the REGION_READOUT_DONE state of this FSM.
Together with the frameReadout() function, this function controls framing and readout, and essentially implements the functions of the Framing and ReadOut Management Unit (FROMU) in the real Alpide chip.

@image html ../img/FROMU_FSM.png "Framing and ReadOut Management (FROMU) FSM"
@image latex ../img/FROMU_FSM.png "Framing and ReadOut Management (FROMU) FSM"


### Data transmission
The dataTransmission() member function is responsible for outputting one 24-bit data word every clock cycle, comma word if no data is available. Also implements a "dummy delay" of configurable length, to account for any delay that is normally associated with encoding and serializing (which will not be implemented in this model).


## PixelMatrix
The PixelMatrix class has an std::queue to represent the Multi Event Buffers (MEB), where the template data type is an std::vector containing 512 PixelDoubleColumn classes, for a total of 1024 pixel columns. The PixelDoubleColumn represents a double column of pixels and the priority encoder in the Alpide chip.

The PixelDoubleColumn contains a std::set where PixelData is the data type, and PixelPriorityEncoder is a friend class for determining the order in which pixels should be read out from the PixelDoubleColumn. Only the pixels that actually have hits will be stored in the std::set, which saves memory and processing time. This implementation is based on the implementation used in a previous Alpide SystemC model by Adam Szczepankiewicz, which was used to determine data rates and find suitable dimensions for FIFOs in the Alpide chip.


## TopReadoutUnit
The Alpide class interfaces with the TRU class purely through the exposed SystemC ports.
The TRU consists of one SystemC SC_METHOD for readout, the topRegionReadoutProcess(), which is sensitive to the rising edge of the 40MHz clock input. This process implements a Finite State Machine (FSM) that controls the framing and readout of data from the regions. This FSM is closely based on the FSM diagrams for the TRU FSM in the real Alpide chip.

@image html ../img/TRU_state_machine.png "TopReadoutunit FSM diagram"
@image latex ../img/TRU_state_machine.png "TopReadoutunit FSM diagram"


## RegionReadoutUnit
Like the TRU class, the interface that the RRU class exposes to the Alpide class is a set of SystemC ports. Internally the RRU consists of one SystemC SC_METHOD, the regionReadoutProcess(), which is sensitive to the rising edge of the 40MHz clock input. Since the double column's priority encoders in the real Alpide chip are based on relatively slow combinatorial logic, there is a setting for matrix readout speed in the real Alpide chip. There are two available settings; half the system clock speed (ie. 20MHz), or one fourth of the clock speed (10MHz). This is also implemented in this SystemC model, where the RRU class divides down the 40MHz clock to the desired matrix readout clock. While most of the logic in the RRU still run on the 40MHz clock, certain readout operations associated with the readout from the pixel matrix will run on the slower (10MHz or 20MHz) clock.

A total of three FSMs are used in the RRU, all 3 reside in separate member functions of the RRU, and are called by the regionReadoutProcess() in order. The 3 FSM functions are:
- regionMatrixReadoutFSM()
- regionValidFSM()
- regionHeaderFSM()

### Region Matrix Readout FSM
The regionMatrixReadoutFSM() is responsible for reading out data from the pixel matrix MEB, and putting it onto the RRU FIFO. When the region in the MEB has been fully read out, a REGION TRAILER word is added to the RRU FIFO. Although the Alpide chip has only 3 MEBs, it can hold many more events than that in its RRU and DMU FIFOs (the last output FIFO). In the RRU FIFO a REGION TRAILER word is added for each event, which allows us to separate between different events in the RRU FIFO.

@image html ../img/RRU_readout_clustering_fsm.png "RegionReadoutUnit Readout and Clustering FSM diagram"
@image latex ../img/RRU_readout_clustering_fsm.png "RegionReadoutUnit Readout and Clustering FSM diagram"

#### Region readout and clustering
Perhaps the most cruical state in the regionMatrixReadoutFSM() is the READOUT AND CLUSTERING state, where pixels are read out from the MEBs, clusters are formed if clustering is enabled, and DATA SHORT or DATA LONG words (LONG for clusters) are placed on the RRU FIFO. The readoutNextPixel() member function of RRU class is responsible for this part.

@image html ../img/RRU_pixel_readout.png "RegionReadoutUnit flowchart for pixel readout and clustering in the readoutNextPixel() member function."
@image latex ../img/RRU_pixel_readout.png "RegionReadoutUnit flowchart for pixel readout and clustering in the readoutNextPixel() member function."

### Region Valid FSM
The regionValidFSM() is used to determine when there is valid data on the RRU FIFO, and when to pop REGION TRAILER words from the RRU FIFO. A valid signal from the RRU is asserted when there is data available on the RRU FIFO, and it is deasserted when the next word on the RRU FIFO is a REGION TRAILER word. When the region is not valid anymore, the regionValidFSM() waits in the POP state for a pop signal from the TRU, which the TRU issues when no RRUs are valid anymore (ie. the current event has been fully read out from the RRU FIFOs). When the pop signal is asserted the TRU pops the REGION TRAILER word from the FIFO.
Note that the REGION TRAILER word is only used internally in the RRU, and disappears when it is popped. It will not, and should not, appear on the data stream out from the Alpide chip.


@image html ../img/RRU_valid_fsm.png "RegionReadoutUnit Valid FSM"
@image latex ../img/RRU_valid_fsm.png "RegionReadoutUnit Valid FSM"

### Region Header FSM
Finally, the regionHeaderFSM() decides if a region header word should be placed on the RRU's data output, or if data from the RRU FIFO should be put on there. When the TRU and RRUs start on a new event, this FSM will ensure that the region header word is outputted first, and data after that.

@image html ../img/RRU_header_fsm.png "RegionReadoutUnit Header FSM"
@image latex ../img/RRU_header_fsm.png "RegionReadoutUnit Header FSM"
