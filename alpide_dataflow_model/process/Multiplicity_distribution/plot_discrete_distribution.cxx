#include <TCanvas.h>
#include <TH1.h>
#include <TGraph.h>
#include <iostream>
#include <fstream>
#include <vector>


void plot_data(const char *filename, const char *plot_name)
{
  TCanvas* c1 = new TCanvas;  
  TH1D* h1 = new TH1D("h1", plot_name, 3000, 0, 3000);
  double num_hits;

  std::ifstream in_file(filename);

  if(in_file.is_open() == false) {
    std::cerr << "Error opening file." << std::endl;
    exit(-1);
  }

  // Read data from file
  while((in_file >> num_hits)) {
    std::cout << "num_hits: " << num_hits << std::endl;

    h1->Fill(num_hits);
  }

  h1->SetXTitle("Hits");
  h1->SetYTitle("Occurences");
  c1->SetLogy(1);
  h1->Draw();

  

  TH1F *h2 = (TH1F*)h1->Clone("h2");
  TCanvas* c2 = new TCanvas;
  c2->SetLogy(1);
  h2->SetYTitle("Probability");
  h2->DrawNormalized();  
}


void plot_discrete_distribution(void)
{
  plot_data("random_hits_multipl_dist_raw_bins.txt", "Plot distribution - raw bins distribution");
  plot_data("random_hits_multipl_dist_fit.txt", "Plot distribution - function fit distribution");
}
