/****************************************************************************
 *
 * Author:
 *
 *  Wagner Carvalho (wcarvalh@cern.ch)
 *
 *  DB builder for PPS geometry
 *
 ****************************************************************************/

#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/ESTransientHandle.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ESWatcher.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/Exception.h"

#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CondCore/DBOutputService/interface/PoolDBOutputService.h"

#include "DetectorDescription/Core/interface/DDCompactView.h"
#include "DetectorDescription/Core/interface/DDFilteredView.h"

#include "DataFormats/CTPPSDetId/interface/TotemRPDetId.h"
#include "DataFormats/CTPPSDetId/interface/TotemTimingDetId.h"
#include "DataFormats/CTPPSDetId/interface/CTPPSPixelDetId.h"
#include "DataFormats/CTPPSDetId/interface/CTPPSDiamondDetId.h"
#include "Geometry/VeryForwardGeometryBuilder/interface/CTPPSDDDNames.h"

#include "Geometry/Records/interface/IdealGeometryRecord.h"
#include "Geometry/Records/interface/VeryForwardIdealGeometryRecord.h"

#include "CondFormats/GeometryObjects/interface/PDetGeomDesc.h"

#include <regex>

//----------------------------------------------------------------------------------------------------

class PPSGeometryBuilder : public edm::one::EDAnalyzer<> {
public:
  explicit PPSGeometryBuilder(const edm::ParameterSet&);
  void analyze(const edm::Event&, const edm::EventSetup&) override;

private:
  void buildPDetGeomDesc(DDFilteredView*, PDetGeomDesc*);
  uint32_t getGeographicalID(DDFilteredView*);

  std::string compactViewTag_;
  edm::ESWatcher<IdealGeometryRecord> watcherIdealGeometry_;
  edm::Service<cond::service::PoolDBOutputService> dbservice_;
};

//----------------------------------------------------------------------------------------------------

PPSGeometryBuilder::PPSGeometryBuilder(const edm::ParameterSet& iConfig)
    : compactViewTag_(iConfig.getUntrackedParameter<std::string>("compactViewTag", "XMLIdealGeometryESSource_CTPPS")) {}

//----------------------------------------------------------------------------------------------------

void PPSGeometryBuilder::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
  edm::ESHandle<DDCompactView> cpv;

  // Get DDCompactView from IdealGeometryRecord
  if (watcherIdealGeometry_.check(iSetup)) {
    iSetup.get<IdealGeometryRecord>().get(compactViewTag_.c_str(), cpv);
  }

  // Create DDFilteredView and apply the filter
  DDPassAllFilter filter;
  DDFilteredView fv((*cpv), filter);

  // Persistent geometry data
  PDetGeomDesc* pdet = new PDetGeomDesc;
  // Build geometry
  buildPDetGeomDesc(&fv, pdet);

  // Save geometry in the database
  if (pdet->container_.empty()) {
    throw cms::Exception("PPSGeometryBuilder") << "PDetGeomDesc is empty, no geometry to save in the database.";
  } else {
    if (dbservice_.isAvailable()) {
      dbservice_->writeOne(pdet, dbservice_->beginOfTime(), "VeryForwardIdealGeometryRecord");
    } else {
      throw cms::Exception("PoolDBService required.");
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------------//----------------------------------------------------------------------------------------------------

void PPSGeometryBuilder::buildPDetGeomDesc(DDFilteredView* fv, PDetGeomDesc* gd) {
  // try to dive into next level
  if (!fv->firstChild())
    return;

  // loop over siblings in the level
  do {
    // Create a PDetGeomDesc::Item node and add it to the parent's (gd) list
    PDetGeomDesc::Item item;

    // =======================================================================
    // Fill Item
    // =======================================================================

    item.dx_ = fv->translation().X();
    item.dy_ = fv->translation().Y();
    item.dz_ = fv->translation().Z();
    const DDRotationMatrix& rot = fv->rotation();
    double xx, xy, xz, yx, yy, yz, zx, zy, zz;
    rot.GetComponents(xx, xy, xz, yx, yy, yz, zx, zy, zz);
    item.axx_ = xx;
    item.axy_ = xy;
    item.axz_ = xz;
    item.ayx_ = yx;
    item.ayy_ = yy;
    item.ayz_ = yz;
    item.azx_ = zx;
    item.azy_ = zy;
    item.azz_ = zz;
    item.name_ = ((fv->logicalPart()).ddname()).name();
    item.params_ = ((fv->logicalPart()).solid()).parameters();
    item.copy_ = fv->copyno();
    item.z_ = fv->geoHistory().back().absTranslation().z();
    // Sensor Type
    item.sensorType_ = "";
    std::string sensor_name = fv->geoHistory().back().logicalPart().name().fullname();
    std::size_t found = sensor_name.find(DDD_CTPPS_PIXELS_SENSOR_NAME);
    if (found != std::string::npos && sensor_name.substr(found - 4, 3) == DDD_CTPPS_PIXELS_SENSOR_TYPE_2x2) {
      item.sensorType_ = DDD_CTPPS_PIXELS_SENSOR_TYPE_2x2;
    }
    // Geographical ID
    item.geographicalID_ = getGeographicalID(fv);

    // =======================================================================

    // add component
    gd->container_.push_back(item);

    // recursion
    buildPDetGeomDesc(fv, gd);
  } while (fv->nextSibling());

  // go a level up
  fv->parent();
}

//----------------------------------------------------------------------------------------------------//----------------------------------------------------------------------------------------------------

uint32_t PPSGeometryBuilder::getGeographicalID(DDFilteredView* view) {
  uint32_t geoID = 0;
  const std::string name = view->logicalPart().name().name();

  // strip sensors
  if (name == DDD_TOTEM_RP_SENSOR_NAME) {
    const std::vector<int>& copy_num = view->copyNumbers();
    // check size of copy numubers array
    if (copy_num.size() < 3)
      throw cms::Exception("DDDTotemRPContruction")
          << "size of copyNumbers for strip sensor is " << copy_num.size() << ". It must be >= 3.";

    // extract information
    const unsigned int decRPId = copy_num[copy_num.size() - 3];
    const unsigned int arm = decRPId / 100;
    const unsigned int station = (decRPId % 100) / 10;
    const unsigned int rp = decRPId % 10;
    const unsigned int detector = copy_num[copy_num.size() - 1];
    geoID = TotemRPDetId(arm, station, rp, detector);
  }

  // strip and pixels RPs
  else if (name == DDD_TOTEM_RP_RP_NAME || name == DDD_CTPPS_PIXELS_RP_NAME) {
    unsigned int decRPId = view->copyno();

    // check if it is a pixel RP
    if (decRPId >= 10000) {
      decRPId = decRPId % 10000;
      const unsigned int armIdx = (decRPId / 100) % 10;
      const unsigned int stIdx = (decRPId / 10) % 10;
      const unsigned int rpIdx = decRPId % 10;
      geoID = CTPPSPixelDetId(armIdx, stIdx, rpIdx);
    } else {
      const unsigned int armIdx = (decRPId / 100) % 10;
      const unsigned int stIdx = (decRPId / 10) % 10;
      const unsigned int rpIdx = decRPId % 10;
      geoID = TotemRPDetId(armIdx, stIdx, rpIdx);
    }
  }

  else if (std::regex_match(name, std::regex(DDD_TOTEM_TIMING_SENSOR_TMPL))) {
    const std::vector<int>& copy_num = view->copyNumbers();
    // check size of copy numbers array
    if (copy_num.size() < 4)
      throw cms::Exception("DDDTotemRPContruction")
          << "size of copyNumbers for TOTEM timing sensor is " << copy_num.size() << ". It must be >= 4.";

    const unsigned int decRPId = copy_num[copy_num.size() - 4];
    const unsigned int arm = decRPId / 100, station = (decRPId % 100) / 10, rp = decRPId % 10;
    const unsigned int plane = copy_num[copy_num.size() - 2], channel = copy_num[copy_num.size() - 1];
    geoID = TotemTimingDetId(arm, station, rp, plane, channel);
  }

  else if (name == DDD_TOTEM_TIMING_RP_NAME) {
    const unsigned int arm = view->copyno() / 100, station = (view->copyno() % 100) / 10, rp = view->copyno() % 10;
    geoID = TotemTimingDetId(arm, station, rp);
  }

  // pixel sensors
  else if (name == DDD_CTPPS_PIXELS_SENSOR_NAME) {
    const std::vector<int>& copy_num = view->copyNumbers();
    // check size of copy numubers array
    if (copy_num.size() < 4)
      throw cms::Exception("DDDTotemRPContruction")
          << "size of copyNumbers for pixel sensor is " << copy_num.size() << ". It must be >= 4.";

    // extract information
    const unsigned int decRPId = copy_num[copy_num.size() - 4] % 10000;
    const unsigned int arm = decRPId / 100;
    const unsigned int station = (decRPId % 100) / 10;
    const unsigned int rp = decRPId % 10;
    const unsigned int detector = copy_num[copy_num.size() - 2] - 1;
    geoID = CTPPSPixelDetId(arm, station, rp, detector);
  }

  // diamond/UFSD sensors
  else if (name == DDD_CTPPS_DIAMONDS_SEGMENT_NAME || name == DDD_CTPPS_UFSD_SEGMENT_NAME) {
    const std::vector<int>& copy_num = view->copyNumbers();

    const unsigned int id = copy_num[copy_num.size() - 1];
    const unsigned int arm = copy_num[1] - 1;
    const unsigned int station = 1;
    const unsigned int rp = 6;
    const unsigned int plane = (id / 100);
    const unsigned int channel = id % 100;

    geoID = CTPPSDiamondDetId(arm, station, rp, plane, channel);
  }

  // diamond/UFSD RPs
  else if (name == DDD_CTPPS_DIAMONDS_RP_NAME) {
    const std::vector<int>& copy_num = view->copyNumbers();

    // check size of copy numubers array
    if (copy_num.size() < 2)
      throw cms::Exception("DDDTotemRPContruction")
          << "size of copyNumbers for diamond RP is " << copy_num.size() << ". It must be >= 2.";

    const unsigned int arm = copy_num[1] - 1;
    const unsigned int station = 1;
    const unsigned int rp = 6;

    geoID = CTPPSDiamondDetId(arm, station, rp);
  }

  return geoID;
}

//----------------------------------------------------------------------------------------------------

DEFINE_FWK_MODULE(PPSGeometryBuilder);
