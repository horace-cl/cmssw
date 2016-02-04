#ifndef EcalPreshowerNoiseDistrib_H
#define EcalPreshowerNoiseDistrib_H

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "DQMServices/Core/interface/DQMStore.h"
#include "FWCore/ServiceRegistry/interface/Service.h"

#include "DataFormats/EcalDigi/interface/ESDataFrame.h"
#include "DataFormats/EcalDigi/interface/EcalDigiCollections.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include "DQMServices/Core/interface/MonitorElement.h"

class EcalPreshowerNoiseDistrib: public edm::EDAnalyzer{

    typedef std::map<uint32_t,float,std::less<uint32_t> >  MapType;

public:

/// Constructor
EcalPreshowerNoiseDistrib(const edm::ParameterSet& ps);

protected:

/// Analyze
void analyze(const edm::Event& e, const edm::EventSetup& c);

private:

 bool verbose_;
 
 DQMStore* dbe_;
 
 std::string outputFile_;

 edm::InputTag ESdigiCollection_;


 MonitorElement* meESDigiADC_[3];
 MonitorElement* meESDigiCorr_[3];
 MonitorElement* meESDigi3D_;
 MonitorElement* meESDigiMultiplicity_;
};

#endif
