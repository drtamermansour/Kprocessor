#include <iostream>
#include <string>
#include "ThirdParty/CLI11.hpp"
#include <vector>
#include <stdint.h>
#include <gqf.h>
#include "KmerCounter/KmerCounter.hpp"
#include "KmerCounter/kmer.h"
#include "Utils/utils.hpp"
#include <cmath>

using namespace std;


int playground_main(int argc, char *argv[]){
  CLI::App app;
  string input_file="";

  app.add_option("-i,--input", input_file,
   "MQF file")->required()->check(CLI::ExistingFile);


  CLI11_PARSE(app, argc, argv);

  QF qf;
  qf_read(&qf,input_file.c_str());
  double res=(double)qf.metadata->noccupied_slots/(double)qf.metadata->xnslots;
  cout<<res<<endl;

  return 0;
}
