import FWCore.ParameterSet.Config as cms

process = cms.Process("PROD")

process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.debugModules = cms.untracked.vstring('*')
process.MessageLogger.destinations = cms.untracked.vstring('cerr')
process.MessageLogger.categories.append('resolution')
process.MessageLogger.cerr =  cms.untracked.PSet(
    threshold = cms.untracked.string('DEBUG'),
    noLineBreaks = cms.untracked.bool(False),
    DEBUG = cms.untracked.PSet(limit = cms.untracked.int32(0)),
    INFO = cms.untracked.PSet(limit = cms.untracked.int32(0)),
    resolution = cms.untracked.PSet(limit = cms.untracked.int32(-1))
)

#process.load("Configuration.StandardSequences.Geometry_cff")
process.load("Configuration.Geometry.GeometryIdeal_cff")
process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")
process.GlobalTag.globaltag = "GR_P_V40::All"

process.load("CondCore.DBCommon.CondDBSetup_cfi")

process.load("DQMServices.Core.DQM_cfg")

process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring(
#        '/store/data/Run2012C/MiniDaq/RAW/v1/000/203/540/AA9053D9-F306-E211-80A4-001D09F248F8.root',
        '/store/data/Run2012C/MiniDaq/RAW/v1/000/199/204/148CF2AC-CAD0-E111-A056-001D09F291D2.root',
###        '/store/data/Run2012C/MiniDaq/RAW/v1/000/200/132/D0E088D3-D7DC-E111-92D2-00237DDC5C24.root',
        '/store/data/Run2012C/MiniDaq/RAW/v1/000/198/510/0C50021A-F4C8-E111-B861-001D09F2512C.root',
        '/store/data/Run2012C/MiniDaq/RAW/v1/000/199/206/EC54DD2C-D3D0-E111-9019-5404A63886CE.root',
        '/store/data/Run2012C/MiniDaq/RAW/v1/000/200/131/E024E67D-D6DC-E111-A404-0025901D6288.root',
        '/store/data/Run2012C/MiniDaq/RAW/v1/000/201/074/92295641-C3E7-E111-899B-0025901D629C.root',
        '/store/data/Run2012C/MiniDaq/RAW/v1/000/200/133/ACB373F1-D9DC-E111-B891-003048F024FE.root',
        '/store/data/Run2012C/MiniDaq/RAW/v1/000/203/276/2CF5C87C-E303-E211-A314-001D09F28F25.root',
        '/store/data/Run2012C/MiniDaq/RAW/v1/000/203/535/1EEFEF95-F506-E211-A872-001D09F2906A.root',
        '/store/data/Run2012C/MiniDaq/RAW/v1/000/200/665/1C75364F-0EE3-E111-8021-BCAEC5329705.root',
        '/store/data/Run2012C/MiniDaq/RAW/v1/000/200/716/8A0AC842-A3E3-E111-A669-001D09F291D7.root'
    )
)

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(-1)
)

process.options = cms.untracked.PSet( wantSummary = cms.untracked.bool(True) )

process.dtunpacker = cms.EDProducer("DTUnpackingModule",
    dataType = cms.string('DDU'),
    inputLabel = cms.InputTag('rawDataCollector'),
    fedbyType = cms.bool(False),
    useStandardFEDid = cms.bool(True),
    dqmOnly = cms.bool(False),                       
    readOutParameters = cms.PSet(
        debug = cms.untracked.bool(False),
        rosParameters = cms.PSet(
            writeSC = cms.untracked.bool(True),
            readingDDU = cms.untracked.bool(True),
            performDataIntegrityMonitor = cms.untracked.bool(False),
            readDDUIDfromDDU = cms.untracked.bool(True),
            debug = cms.untracked.bool(False),
            localDAQ = cms.untracked.bool(False)
        ),
        localDAQ = cms.untracked.bool(False),
        performDataIntegrityMonitor = cms.untracked.bool(False)
    )
)

process.PoolDBOutputService = cms.Service("PoolDBOutputService",
    process.CondDBSetup,
    timetype = cms.untracked.string('runnumber'),
    connect = cms.string('sqlite_file:t0.db'),
    authenticationMethod = cms.untracked.uint32(0),
    toPut = cms.VPSet(cms.PSet(
        record = cms.string('DTT0Rcd'),
        tag = cms.string('t0')
    ))
)

process.eventInfoProvider = cms.EDFilter("EventCoordinatesSource",
    eventInfoFolder = cms.untracked.string('EventInfo/')
)

# test pulse monitoring
process.load("DQM.DTMonitorModule.dtDigiTask_TP_cfi")
process.load("DQM.DTMonitorClient.dtOccupancyTest_TP_cfi")
process.dtTPmonitor.readDB = False 
process.dtTPmonitor.defaultTtrig = 600
process.dtTPmonitor.defaultTmax = 100
process.dtTPmonitor.inTimeHitsLowerBound = 0
process.dtTPmonitor.inTimeHitsUpperBound = 0
file = open("tpDead.txt")
wiresToDebug = cms.untracked.vstring()
for line in file:
    corrWire = line.split()[:6]
    #switch station/sector
    corrWire[1:3] = corrWire[2:0:-1]
    wire = ' '.join(corrWire)
    #print wire
    wiresToDebug.append(wire)
file.close()

process.dtT0WireCalibration = cms.EDAnalyzer("DTT0Calibration",
    correctByChamberMean = cms.bool(False),
    # Cells for which you want the histos (default = None)
    cellsWithHisto = wiresToDebug,
    # Label to retrieve DT digis from the event
    digiLabel = cms.untracked.string('dtunpacker'),
    calibSector = cms.untracked.string('All'),
    # Chose the wheel, sector (default = All)
    calibWheel = cms.untracked.string('All'),
    # Number of events to be used for the t0 per layer histos
    eventsForWireT0 = cms.uint32(25000),  #25000
    # Name of the ROOT file which will contain the test pulse times per layer
    rootFileName = cms.untracked.string('DTTestPulses.root'),
    debug = cms.untracked.bool(False),
    rejectDigiFromPeak = cms.uint32(50),
    # Acceptance for TP peak width
    tpPeakWidth = cms.double(15.0),
    # Number of events to be used for the t0 per layer histos
    eventsForLayerT0 = cms.uint32(5000)  #5000
)

process.output = cms.OutputModule("PoolOutputModule",
    outputCommands = cms.untracked.vstring('drop *', 
                                           'keep *_MEtoEDMConverter_*_*'),
    fileName = cms.untracked.string('DQM.root')
)

process.load("DQMServices.Components.MEtoEDMConverter_cff")
process.DQM.collectorHost = ''
"""
process.load("DQMServices.Components.DQMEnvironment_cfi")
process.DQMStore.referenceFileName = ''
process.dqmSaver.convention = 'Offline'
process.dqmSaver.workflow = '/MiniDaq/HIRun2010-v1-dtCalibration-rev1/RAW'
process.DQMStore.collateHistograms = False
process.dqmSaver.convention = "Offline"
"""

process.p = cms.Path(process.dtunpacker*
                     process.dtTPmonitor+process.dtTPmonitorTest+
                     process.dtT0WireCalibration+
                     process.MEtoEDMConverter)
process.outpath = cms.EndPath(process.output)
