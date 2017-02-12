#include "octdatamanager.h"
#include <data_structure/programoptions.h>

#include <iostream>

#include <QString>
#include <QTime>

#include "octmarkerio.h"


#include <octdata/datastruct/series.h>
#include <octdata/datastruct/bscan.h>
#include <octdata/datastruct/oct.h>
#include <octdata/octfileread.h>
#include <octdata/filereadoptions.h>

#include <helper/ptreehelper.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <boost/lexical_cast.hpp>

#include <boost/exception/diagnostic_information.hpp>
#include <boost/filesystem.hpp>

namespace bpt = boost::property_tree;
namespace bfs = boost::filesystem;



OctDataManager::OctDataManager()
: markerstree(new bpt::ptree)
, markerIO(new OctMarkerIO(markerstree))
{
}



OctDataManager::~OctDataManager()
{
	delete octData;
	delete markerstree;
	delete markerIO;
}


void OctDataManager::saveMarkersDefault()
{
	if(ProgramOptions::autoSaveOctMarkers())
	{
		saveMarkerState(actSeries);
		markerIO->saveDefaultMarker(actFilename.toStdString());
	}
}

void OctDataManager::triggerSaveMarkersDefault()
{
	saveMarkerState(actSeries);
	markerIO->saveDefaultMarker(actFilename.toStdString());
}


void OctDataManagerThread::run()
{
	if(!oct)
	{
		error = "oct variable is not set";
		loadSuccess = false;
	}

	try
	{
		OctData::FileReadOptions octOptions;
		octOptions.e2eGray             = static_cast<OctData::FileReadOptions::E2eGrayTransform>(ProgramOptions::e2eGrayTransform());
		octOptions.registerBScanns     = ProgramOptions::registerBScans();
		octOptions.fillEmptyPixelWhite = ProgramOptions::fillEmptyPixelWhite();
		octOptions.holdRawData         = ProgramOptions::holdOCTRawData();

		*oct = std::move(OctData::OctFileRead::openFile(filename.toStdString(), octOptions, this));
	}
	catch(boost::exception& e)
	{
		error = QString::fromStdString(boost::diagnostic_information(e));
		loadSuccess = false;
	}
	catch(std::exception& e)
	{
		error = QString::fromStdString(e.what());
		loadSuccess = false;
	}
	catch(const char* str)
	{
		error = str;
		loadSuccess = false;
	}
	catch(...)
	{
		error = QString("Unknow error in file %1 line %2").arg(__FILE__).arg(__LINE__);
		loadSuccess = false;
	}
}

void OctDataManager::openFile(const QString& filename)
{
	if(loadThread)
		return;

	loadFileSignal(true);
	saveMarkersDefault();

	octData4Loading = new OctData::OCT;

	loadThread = new OctDataManagerThread(*this, filename, octData4Loading);
	connect(loadThread, &OctDataManagerThread::stepCalulated, this, &OctDataManager::loadOctDataThreadProgress);
	connect(loadThread, &OctDataManagerThread::finished     , this, &OctDataManager::loadOctDataThreadFinish  );
	loadThread->start();
}


void OctDataManager::loadOctDataThreadFinish()
{
	loadFileSignal(false);

	if(loadThread->success())
	{
		if(octData4Loading->size() == 0)
			throw "OctDataManager::openFile: oct->size() == 0";

		delete octData;
		octData = octData4Loading;
		octData4Loading = nullptr;

		actPatient = octData->begin()->second;
		if(actPatient->size() > 0)
		{
			actStudy = actPatient->begin()->second;

			if(actStudy->size() > 0)
			{
				actSeries = actStudy->begin()->second;
			}
		}

		actFilename = loadThread->getFilename();


		markerstree->clear();

		markerIO->loadDefaultMarker(actFilename.toStdString());

		loadFileSignal(false);

		emit(octFileChanged(octData   ));
		emit(patientChanged(actPatient));
		emit(studyChanged  (actStudy  ));
		emit(seriesChanged (actSeries ));
	}
	else
	{
		QString loadError = loadThread->getError();

		if(!loadError.isEmpty())
		{
			delete loadThread;
			delete octData4Loading;
			loadThread      = nullptr;
			octData4Loading = nullptr;

			throw loadError;
		}
	}

	delete loadThread;
	delete octData4Loading;
	loadThread      = nullptr;
	octData4Loading = nullptr;
}



void OctDataManager::chooseSeries(const OctData::Series* seriesReq)
{
	saveMarkerState(actSeries);
	
	
	if(seriesReq == actSeries)
		return;
	
	const OctData::Patient* patient;
	const OctData::Study  * study;
	octData->findSeries(seriesReq, patient, study);

	if(!patient || !study)
		return;

	actPatient = patient;
	emit(patientChanged(patient));

	actStudy = study;
	emit(studyChanged(study));

	actSeries = seriesReq;
	emit(seriesChanged(seriesReq));
}


boost::property_tree::ptree* OctDataManager::getMarkerTreeSeries(const OctData::Series* seriesReq)
{
	if(!seriesReq)
		return nullptr;
	const OctData::Patient* patient;
	const OctData::Study  * study;
	octData->findSeries(seriesReq, patient, study);

	return getMarkerTreeSeries(patient, study, seriesReq);
}

boost::property_tree::ptree* OctDataManager::getMarkerTreeSeries(const OctData::Patient* patient, const OctData::Study* study, const OctData::Series* series)
{
	if(!patient || !study || !series)
		return nullptr;

	bpt::ptree& patNode    = PTreeHelper::getNodeWithId(*markerstree, "Patient", patient->getInternalId());
	bpt::ptree& studyNode  = PTreeHelper::getNodeWithId(patNode     , "Study"  , study  ->getInternalId());
	bpt::ptree& seriesNode = PTreeHelper::getNodeWithId(studyNode   , "Series" , series ->getInternalId());


	PTreeHelper::putNotEmpty(patNode   , "PatientUID", patient->getPatientUID());
	PTreeHelper::putNotEmpty(studyNode , "StudyUID"  , study  ->getStudyUID  ());
	PTreeHelper::putNotEmpty(seriesNode, "SeriesUID" , series ->getSeriesUID ());


	return &seriesNode;
}


/*
bool OctDataManager::addMarkers(QString filename, OctDataManager::Fileformat format)
{
	return false;
}
*/

bool OctDataManager::loadMarkers(QString filename, OctMarkerFileformat format)
{
	markerstree->clear();
	markerIO->loadMarkers(filename.toStdString(), format);
	emit(loadMarkerStateAll());
	return true;
}

void OctDataManager::saveMarkers(QString filename, OctMarkerFileformat format)
{
	saveMarkerState(actSeries);
	markerIO->saveMarkers(filename.toStdString(), format);
}

