// -*- C++ -*-
//
// Package:    SoftLepton
// Class:      SoftLepton
// 
/**\class SoftLepton SoftLepton.cc RecoBTag/SoftLepton/src/SoftLepton.cc

 Description: CMSSW EDProducer wrapper for sot lepton b tagging.

 Implementation:
     The actual tagging is performed by SoftLeptonAlgorithm.
*/
//
// Original Author:  fwyzard
//         Created:  Wed Oct 18 18:02:07 CEST 2006
// $Id: SoftLepton.cc,v 1.21 2007/05/30 07:37:38 arizzi Exp $
//


#include <memory>
#include <string>
#include <iostream>

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/MuonReco/interface/Muon.h"
#include "DataFormats/EgammaCandidates/interface/Electron.h"
#include "DataFormats/BTauReco/interface/SoftLeptonTagInfo.h"

#include "TrackingTools/TransientTrack/interface/TransientTrackBuilder.h"
#include "TrackingTools/Records/interface/TransientTrackRecord.h"
#include "RecoVertex/PrimaryVertexProducer/interface/PrimaryVertexSorter.h"
#include "RecoBTau/JetTagComputer/interface/JetTagComputer.h"
#include "RecoBTau/JetTagComputer/interface/JetTagComputerRecord.h"
#include "RecoBTag/BTagTools/interface/SignedImpactParameter3D.h"
#include "RecoBTag/SoftLepton/interface/SoftLepton.h"

using namespace std;
using namespace edm;
using namespace reco;

const reco::Vertex SoftLepton::s_nominalBeamSpot(
  reco::Vertex::Point( 0, 0, 0 ),
  reco::Vertex::Error( ROOT::Math::SVector<double,6>( 0.0015 * 0.0015, //          0.0,        0.0
                                                                  0.0, 0.0015 * 0.0015, //     0.0  
                                                                  0.0,             0.0, 15. * 15. ) ),
  1, 1, 0 );

SoftLepton::SoftLepton(const edm::ParameterSet & iConfig) :
  m_jets(          iConfig.getParameter<edm::InputTag>( "jets" ) ),
  m_primaryVertex( iConfig.getParameter<edm::InputTag>( "primaryVertex" ) ),
  m_leptons(       iConfig.getParameter<edm::InputTag>( "leptons" ) ),
  m_algo(          iConfig.getParameter<edm::ParameterSet> ( "algorithmConfiguration") )
{
  produces<reco::SoftLeptonTagInfoCollection>();
}

SoftLepton::~SoftLepton() {
}

void
SoftLepton::produce(edm::Event& iEvent, const edm::EventSetup& iSetup) {

  // input objects

  // input jets (and possibly tracks)
  std::vector<edm::RefToBase<reco::Jet> > jets;
  std::vector<reco::TrackRefVector>       tracks;
  try {
    Handle<reco::JetTracksAssociationCollection> h_jtas;
    iEvent.getByLabel(m_jets, h_jtas);

    unsigned int size = h_jtas->size();
    jets.resize(size);
    tracks.resize(size);
    for (unsigned int i = 0; i < size; ++i) {
      jets[i]   = (*h_jtas)[i].first;
      tracks[i] = (*h_jtas)[i].second;
    }
  }
  catch(edm::Exception e) {
    Handle<reco::CaloJetCollection> h_jets;
    iEvent.getByLabel(m_jets, h_jets);

    unsigned int size = h_jets->size();
    jets.resize(size);
    tracks.resize(size);
    for (unsigned int i = 0; i < h_jets->size(); i++)
      jets[i] = edm::RefToBase<reco::Jet>( reco::CaloJetRef(h_jets, i) );
  }
  
  // input primary vetex (optional, can be "none")
  reco::Vertex vertex;
  Handle<reco::VertexCollection> h_primaryVertex;
  if (m_primaryVertex.label() != "none")
    iEvent.getByLabel(m_primaryVertex, h_primaryVertex);

  if (h_primaryVertex->size()) {
    PrimaryVertexSorter pvs;
    vertex = pvs.sortedList(*(h_primaryVertex.product())).front();
  } else {
    vertex = s_nominalBeamSpot;
  }

  // input leptons (can be of different types)
  reco::TrackRefVector leptons;
  // try to access the input collection as a collection of Electons, Muons or Tracks
  // FIXME: it would be nice not to have to rely on exceptions
  try {
    Handle<reco::ElectronCollection> h_electrons;
    iEvent.getByLabel(m_leptons, h_electrons);
    #ifdef DEBUG
    cerr << "SoftLepton::produce : collection " << m_leptons << " found, identified as ElectronCollection" << endl;
    #endif
    for (reco::ElectronCollection::const_iterator electron = h_electrons->begin(); electron != h_electrons->end(); ++electron)
      leptons.push_back(electron->track());
  } 
  catch(edm::Exception e)  { 
    // electrons not found, look for muons
    try {
      Handle<reco::MuonCollection> h_muons;
      iEvent.getByLabel(m_leptons, h_muons);
      #ifdef DEBUG
      cerr << "SoftLepton::produce : collection " << m_leptons << " found, identified as MuonCollection" << endl;
      #endif
      for (reco::MuonCollection::const_iterator muon = h_muons->begin(); muon != h_muons->end(); ++muon)
        if(! muon->combinedMuon().isNull() )
          leptons.push_back( muon->combinedMuon() );
        else 
          cerr << "SoftLepton::produce : found a Null edm::Ref in MuonCollection " << m_leptons << ", skipping it" << endl;
    }
    catch(edm::Exception e) {
      // electrons or muons not found, look for tracks
      Handle<reco::TrackCollection> h_tracks;
      iEvent.getByLabel(m_leptons, h_tracks);
      #ifdef DEBUG
      cerr << "SoftLepton::produce : collection " << m_leptons << " found, identified as TrackCollection" << endl;
      #endif
      for (unsigned int i = 0; i < h_tracks->size(); i++)
        leptons.push_back( TrackRef(h_tracks, i) );
    }
  }

  // output collections
  std::auto_ptr<reco::SoftLeptonTagInfoCollection> outputCollection(  new reco::SoftLeptonTagInfoCollection() );

  #ifdef DEBUG
  std::cerr << std::endl;
  std::cerr << "Found " << jets.size() << " jets:" << std::endl;
  #endif // DEBUG
  for (unsigned int i = 0; i < jets.size(); ++i) {
    reco::SoftLeptonTagInfo result = m_algo.tag( jets[i], tracks[i], leptons, vertex );
    #ifdef DEBUG
//    std::cerr << "  Jet " << std::setw(2) << i << " has " << std::setw(2) << result.first.tracks().size() << " tracks and " << std::setw(2) << result.second.leptons() << " leptons" << std::endl;
  //  std::cerr << "  Tagger result: " << result.first.discriminator() << endl;
    #endif // DEBUG
    outputCollection->push_back( result );
  }

  iEvent.put( outputCollection );
}

// ------------ method called once each job just before starting event loop  ------------
void 
SoftLepton::beginJob(const edm::EventSetup& iSetup) {
  // grab a TransientTrack helper from the Event Setup
  edm::ESHandle<TransientTrackBuilder> builder;
  iSetup.get<TransientTrackRecord>().get( "TransientTrackBuilder", builder );
  m_algo.setTransientTrackBuilder( builder.product() );
}

// ------------ method called once each job just after ending the event loop  ------------
void 
SoftLepton::endJob(void) {
}

