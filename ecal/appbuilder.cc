/*============================================================================
Copyright 2017 Koichi Murakami

Distributed under the OSI-approved BSD License (the "License");
see accompanying file LICENSE for details.

This software is distributed WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the License for more information.
============================================================================*/
#include <vector>
#include <boost/lexical_cast.hpp>
#include "G4Navigator.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4TransportationManager.hh"
#include "G4UImanager.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VSolid.hh"
#include "FTFP_BERT.hh"
#include "appbuilder.h"
#include "ecalgeom.h"
#include "particlegun.h"
#include "util/jsonparser.h"
//#include "analyzer.h"
#include "eventaction.h"
#include "runaction.h"
//#include "simdata.h"

// --------------------------------------------------------------------------
namespace {

G4RunManager* run_manager = nullptr;
G4UImanager* ui_manager = nullptr;
JsonParser* jparser = nullptr;
//Analyzer* analyzer = NULL;

// --------------------------------------------------------------------------
void SetupGeomtry()
{
  EcalGeom* geom = new EcalGeom();
  run_manager-> SetUserInitialization(geom);
}

// --------------------------------------------------------------------------
G4ThreeVector GetPrimaryPosition()
{
  G4ThreeVector pos = G4ThreeVector();
  if ( jparser-> Contains("Primary/position") ) {
    std::vector<double> dvec;
    dvec.clear();
    jparser-> GetDoubleArray("Primary/position", dvec);
    pos = G4ThreeVector(dvec[0]*cm, dvec[1]*cm, dvec[2]*cm);
  }
  return pos;
}

// --------------------------------------------------------------------------
void SetupParticleGun()
{
  ParticleGun* pga = new ParticleGun();
  run_manager-> SetUserAction(pga);
  G4ParticleGun* gun = pga-> GetGun();

  std::string pname = jparser-> GetStringValue("Primary/particle");
  G4ParticleTable* ptable = G4ParticleTable::GetParticleTable();
  G4ParticleDefinition* pdef = ptable-> FindParticle(pname);
  if ( pdef != nullptr ) gun-> SetParticleDefinition(pdef);

  double pkin = jparser-> GetDoubleValue("Primary/energy");
  gun-> SetParticleEnergy(pkin*MeV);

  std::vector<double> dvec;
  if ( jparser-> Contains("Primary/direction") ) {
    dvec.clear();
    jparser-> GetDoubleArray("Primary/direction", dvec);
    G4ThreeVector pvec(dvec[0], dvec[1], dvec[2]);
    gun-> SetParticleMomentumDirection(pvec);
  }

  G4ThreeVector pos = GetPrimaryPosition();
  gun-> SetParticlePosition(pos);
}

// --------------------------------------------------------------------------
void SetupPGA()
{
  std::string primary_type = jparser-> GetStringValue("Primary/type");
  if ( primary_type == "gun" ) {
    std::cout << "[ MESSAGE ] primary type : gun" << std::endl;
    SetupParticleGun();
  } else if ( primary_type == "beam" ) {
    std::cout << "[ MESSAGE ] primary type : beam" << std::endl;
    //...
  } else {
    std::cout << "[ MESSAGE ] primary type : gun" << std::endl;
    SetupParticleGun();
  }
}

} // end of namespace

// ==========================================================================
AppBuilder::AppBuilder()
{
  ::run_manager = G4RunManager::GetRunManager();
  ::ui_manager = G4UImanager::GetUIpointer();
  ::jparser = JsonParser::GetJsonParser();

  //simdata_ = new SimData;
  //::analyzer = Analyzer::GetAnalyzer();
  //::analyzer-> SetSimData(simdata_);
}

// --------------------------------------------------------------------------
AppBuilder::~AppBuilder()
{
  //delete simdata_;
}

// --------------------------------------------------------------------------
void AppBuilder::SetupApplication()
{
  ::SetupGeomtry();
  ::run_manager-> SetUserInitialization(new FTFP_BERT);
  ::SetupPGA();

  RunAction* runaction = new RunAction;
  ::run_manager-> SetUserAction(runaction);

  EventAction* eventaction = new EventAction;
  ::run_manager-> SetUserAction(eventaction);

  // initialize analyzer (allocate data memory)
  //::analyzer-> Initialize();

  ::run_manager-> Initialize();

  G4ThreeVector pos = ::GetPrimaryPosition();
  bool qcheck = CheckVPrimaryPosition(pos);
  if ( qcheck == false ) {
    std::cout << "[ ERROR ] primary position out of world." << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

// --------------------------------------------------------------------------
bool AppBuilder::CheckVPrimaryPosition(const G4ThreeVector& pos)
{
  G4Navigator* navigator = G4TransportationManager::GetTransportationManager()
                             -> GetNavigatorForTracking();

  G4VPhysicalVolume* world = navigator-> GetWorldVolume();
  G4VSolid* solid = world-> GetLogicalVolume()-> GetSolid();
  EInside qinside = solid-> Inside(pos);

   if( qinside != kInside) return false;
   else return true;
}
