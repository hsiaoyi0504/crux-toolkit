/**
 * \file PredictMPSMIons.h 
 * AUTHOR: Sean McIlwain
 * CREATE DATE: 6 December 2010
 * \brief Object for running search-for-xlinks
 *****************************************************************************/

#ifndef SCOREMPSMSPECTRUM_H
#define SCOREMPSMSPECTRUM_H

#include "CruxApplication.h"
#include "DelimitedFileReader.h"

#include <string>

class ScoreMPSMSpectrum: public CruxApplication {

 public:

  ScoreMPSMSpectrum();
  ~ScoreMPSMSpectrum();
  virtual int main(int argc, char** argv);
  virtual std::string getName();
  virtual std::string getDescription();

};


#endif
