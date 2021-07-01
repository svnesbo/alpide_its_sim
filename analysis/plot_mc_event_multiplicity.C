#include <TCanvas.h>
//#include <TH1F.h>
#include <TH1I.h>
#include <TFile.h>
#include <TROOT.h>
#include <TTree.h>
//#include <QDir>


#include <cstring>
#include <iostream>
#include <string>
/*
#include <cmath>
#include <fstream>
#include <iomanip>
#include <vector>
*/



//const std::string csv_delim(";");

// Required by PixelHit.hpp
std::int64_t g_num_pixels_in_mem = 0;

int plot_mc_events_multiplicity(const char* path_to_root_file,
                                const char* output_path,
                                Int_t num_bins)
{
  TFile f(path_to_root_file, "read");

  TTree* T = (TTree*) f.Get("event_multiplicity");

  TCanvas* c1 = new TCanvas();
  c1->cd();

  TH1I* h[7];

  for(int i = 0; i < 7; i++)
  {
    h[i] = new TH1I(Form("h%d", i), Form("Event multiplicity - layer %d", i), num_bins, 0, 0);
  }

  // Create plots for each layer
  for(int lay_num = 0; lay_num < 7; lay_num++)
  {
    T->Draw(Form("layer_%d >> h%d", lay_num, lay_num));

    h[lay_num]->GetYaxis()->SetTitle("Counts");
    h[lay_num]->GetXaxis()->SetTitle("Pixel hit multiplicity");

    c1->Update();
    c1->SetLogy(0);
    c1->Print(Form("%s/multiplicity_layer%d.png", output_path, lay_num));
    c1->Print(Form("%s/multiplicity_layer%d.pdf", output_path, lay_num));
    c1->SetLogy(1);
    c1->Print(Form("%s/multiplicity_layer%d_log.png", output_path, lay_num));
    c1->Print(Form("%s/multiplicity_layer%d_log.pdf", output_path, lay_num));
  }

  // Create a plot that contains all layers
  h[0]->Draw();
  for(int lay_num = 1; lay_num < 7; lay_num++)
  {
    h[lay_num]->Draw("SAME");
  }
  c1->Update();
  c1->SetLogy(0);
  c1->Print(Form("%s/multiplicity_all.png", output_path));
  c1->Print(Form("%s/multiplicity_all.pdf", output_path));
  c1->SetLogy(1);
  c1->Print(Form("%s/multiplicity_log_all.png", output_path));
  c1->Print(Form("%s/multiplicity_log_all.pdf", output_path));

  return 0;
}


void print_help(void)
{
  std::cout << std::endl;
  std::cout << "Plot histograms of multiplicities for each layer of" << std::endl;
  std::cout << "the MC event data for ITS in the SystemC simulations, using" << std::endl;
  std::cout << "data from a .root file created with get_mc_event_multiplicity" << std::endl;
  std::cout << std::endl;
  std::cout << "Usage:" << std::endl;
  std::cout << "plot_mc_events_multiplicity <path_to_root_file> <output_path> <num_bins>" << std::endl;
}


# ifndef __CINT__
int main(int argc, char** argv)
{
  if(argc != 4) {
    print_help();
    exit(0);
  }

  Int_t num_bins = atoi(argv[3]);

  return plot_mc_events_multiplicity(argv[1], argv[2], num_bins);
}
# endif
