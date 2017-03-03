/**
 * @file   vcd_trace.h
 * @author Simon Voigt Nesbo
 * @date   March 3, 2017
 * @brief  Common function for adding SystemC signals etc. to Value Change Dump (VCD) file
 *
 */

#ifndef VCD_TRACE_H
#define VCD_TRACE_H

#include <systemc.h>
#include <string>
#include <sstream>

///@brief Add a SystemC signal/trace to VCD file, with desired signal hierarchy given by name_prefix.
///@param wf VCD waveform file pointer
///@param name_prefix Prefix to be added before signal name, used for signal hierarchy.
///                   A period (.) separates levels of hierarchy.
///@param signal_name Name of the signal
///@param signal The SystemC signal object
template<class T> static inline void addTrace(sc_trace_file *wf, std::string name_prefix, std::string signal_name, T& signal)
{
  std::stringstream ss;
  ss << name_prefix << signal_name;
  std::string str_signal_full_path_name(ss.str());
  sc_trace(wf, signal, str_signal_full_path_name);
}

#endif
