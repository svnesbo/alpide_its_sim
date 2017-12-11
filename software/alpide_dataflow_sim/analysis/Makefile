CXX=`root-config --cxx`
CXXFLAGS=`root-config --cflags`
LDFLAGS=`root-config --ldflags`
LDLIBS=`root-config --glibs`
ROOTLIBS=-lRooFit -lHtml -lMinuit -lRooFitCore -lRooStats -lHistFactory


# Assumes that the script is in MyScript.C
# and it must contain a main() function

# all: process_readout_trigger_stats

# %.o : %.C
# 	g++ -g -Wall `root-config --cflags --libs` -L$(ROOTSYS)/lib $(ROOTLIBS) $? -o $@

# %.o : %.cpp
# 	g++ -g -Wall `root-config --cflags --libs` -L$(ROOTSYS)/lib $(ROOTLIBS) $? -o $@

# %.o : %.cc
# 	g++ -g -Wall `root-config --cflags --libs` -L$(ROOTSYS)/lib $(ROOTLIBS) $? -o $@




# IDIR =../include
# CC=gcc
# CFLAGS=-I$(IDIR)

# ODIR=obj
# LDIR =../lib

# LIBS=-lm

DEPS = ITSLayerStats.hpp LinkStats.hpp ReadoutUnitStats.hpp
#DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

OBJ = ITSLayerStats.o ReadoutUnitStats.o process_readout_trigger_stats.o
#OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


%.o: %.C %.cpp $(DEPS)
	$(CXX) -c -g -Wall $(CXXFLAGS) -o $@ $<

process_readout_trigger_stats: $(OBJ)
	$(CXX) -g -Wall -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(LDLIBS) -L$(ROOTSYS)/lib $(ROOTLIBS)
#gcc -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o *~ core
