#ifndef UTILS_HPP
#define UTILS_HPP

#include <boost/current_function.hpp>
#include <iostream>

#define print_function_timestamp()                                      \
  std::cout << std::endl << "@ " << sc_time_stamp().value() << " ns\t"; \
  std::cout << BOOST_CURRENT_FUNCTION << ":" << std::endl;              \
  std::cout << "-------------------------------------------";           \
  std::cout << "-------------------------------------------" << std::endl;


#endif
