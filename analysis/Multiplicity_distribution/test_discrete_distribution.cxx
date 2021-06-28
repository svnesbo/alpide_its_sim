#include <boost/random/mersenne_twister.hpp>
#include <boost/random/discrete_distribution.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>


void test_distribution(const char *distribution_filename, int num_events)
{
  std::vector<double> v;
  std::string s1(distribution_filename);
  std::string s2("random_hits_");
  std::string output_filename = s2 + s1;
  
  std::ifstream in_file(distribution_filename);

  if(in_file.is_open() == false) {
    std::cerr << "Error opening input file." << std::endl;
    exit(-1);
  }

  int i = 0;
  int x;
  double y;
  
  while((in_file >> x >> y)) {
    std::cout << "x: " << x << std::endl;
    std::cout << "y: " << y << std::endl;

    // Some bins/x-values may be missing in the file.
    // Missing bins have zero probability, but need to be present in the
    // vector because discrete_distribution expects the full range
    while(i < x) {
      v.push_back(0);
      i++;
    }

    v.push_back(y);
    i++;
  }

  in_file.close();

  std::cout << "\nPrinting vector: \n";
  for(i = 0; i < v.size(); i++)
    std::cout << i << ": " << v[i] << std::endl;

  
  boost::random::mt19937 gen;
  boost::random::discrete_distribution<> dist(v.begin(), v.end());  

  std::ofstream out_file(output_filename.c_str());

  if(out_file.is_open() == false) {
    std::cerr << "Error opening output file." << std::endl;
    exit(-1);
  }  
  
  for(int event_count = 0; event_count < num_events; event_count++) {
    int num_hits = dist(gen);
    out_file << num_hits << std::endl;
    std::cout << "num_hits: " << num_hits << std::endl;
  }
  out_file.close();
}


int main(int argc, char **argv)
{
  const int num_events = 1000000;

  test_distribution("multipl_dist_raw_bins.txt", num_events);
  test_distribution("multipl_dist_fit.txt", num_events);  
}
