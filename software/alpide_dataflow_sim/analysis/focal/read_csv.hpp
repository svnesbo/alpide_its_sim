#ifndef READ_CSV_HPP
#define READ_CSV_HPP

#include <map>
#include <string>
#include <vector>

std::vector<std::map<std::string, unsigned long> > read_csv(std::string csv_file_path,
                                                            const char delim,
                                                            bool skip_ws = true);

#endif
