import FWCore.ParameterSet.Config as cms

from IOMC.EventVertexGenerators.VtxSmearedParameters_cfi import *
myVertexGenerator = cms.PSet(
    NominalCollision4VtxSmearingParameters,
    type = cms.string('BetaFunc')
)

