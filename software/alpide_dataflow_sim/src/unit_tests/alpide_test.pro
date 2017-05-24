######################################################################
# Automatically generated by qmake (3.0) man. des. 12 15:18:45 2016
######################################################################

CONFIG += debug c++11
QT += xml
TEMPLATE = app
TARGET = alpide_test
INCLUDEPATH += .
INCLUDEPATH += $(SYSTEMC_HOME)/include
OBJECTS_DIR = build
DEFINES += SC_INCLUDE_DYNAMIC_PROCESSES


LIBS += -L$(SYSTEMC_HOME)/lib-linux64
LIBS += -lsystemc -lm -lboost_random

QMAKE_CXXFLAGS += -O2
QMAKE_CFLAGS += -O2

# Input
HEADERS += ../alpide/alpide_constants.h \
           ../alpide/alpide_data_format.h \
           ../alpide/alpide_data_parser.h \
           ../alpide/alpide.h \
           ../alpide/pixel_col.h \
           ../alpide/pixel_matrix.h \
           ../alpide/region_readout.h \
           ../alpide/top_readout.h \
           ../event/trigger_event.h \
           ../event/hit.h \
           ../misc/vcd_trace.h
SOURCES += ../alpide/alpide.cpp \
           ../alpide/alpide_data_parser.cpp \
           ../alpide/pixel_col.cpp \
           ../alpide/pixel_matrix.cpp \
           ../alpide/region_readout.cpp \
           ../alpide/top_readout.cpp \
           ../event/trigger_event.cpp \
           ../event/hit.cpp \
           alpide_test.cpp
