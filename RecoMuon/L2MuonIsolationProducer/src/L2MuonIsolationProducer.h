#ifndef RecoMuon_L2MuonIsolationProducer_H
#define RecoMuon_L2MuonIsolationProducer_H

/**  \class L2MuonIsolationProducer
 * 
 *   L2 HLT muon isolation producer
 *
 *   \author  J.Alcaraz
 */

#include "FWCore/Framework/interface/EDProducer.h"
#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/Event.h"

#include "PhysicsTools/IsolationAlgos/interface/IsoDepositExtractor.h"
#include "RecoMuon/MuonIsolation/interface/MuIsoBaseIsolator.h"

class L2MuonIsolationProducer : public edm::EDProducer {

 public:

  /// constructor with config
  L2MuonIsolationProducer(const edm::ParameterSet&);
  
  /// destructor
  virtual ~L2MuonIsolationProducer(); 

  /// setup the job  
  virtual void beginJob();

  /// Produce isolation maps
  virtual void produce(edm::Event&, const edm::EventSetup&);
  // ex virtual void reconstruct();

 private:
  
  // Muon track Collection Label
  edm::InputTag theSACollectionLabel;

  // Option to write MuIsoDeposits into the event
  bool optOutputDecision;

  // MuIsoExtractor
  reco::isodeposit::IsoDepositExtractor* theExtractor;

  // muon isolator 
  muonisolation::MuIsoBaseIsolator * theDepositIsolator;

};

#endif
