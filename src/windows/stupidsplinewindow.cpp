#include "stupidsplinewindow.h"

#include <QMenu>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QtGui>

#include <QProgressDialog>
#include <QLabel>

#include <widgets/sloimagewidget.h>
#include <widgets/bscanmarkerwidget.h>
#include <widgets/dwmarkerwidgets.h>
#include <widgets/scrollareapan.h>

#include <data_structure/programoptions.h>

#include <manager/octmarkermanager.h>
#include <manager/octdatamanager.h>
#include <manager/octmarkerio.h>
#include <markermodules/bscanmarkerbase.h>

#include <model/octfilesmodel.h>
#include <model/octdatamodel.h>

#include <octdata/datastruct/sloimage.h>
#include <octdata/datastruct/bscan.h>
#include <octdata/datastruct/series.h>

#include <octdata/octfileread.h>
#include <octdata/filereadoptions.h>

#include <boost/exception/exception.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include <globaldefinitions.h>

#include <QPushButton>
#include <widgets/dwimagecoloradjustments.h>
#include <QToolButton>

#include<windows/infodialogs.h>
#include <markermodules/bscanlayersegmentation/bscanlayersegmentation.h>


namespace
{
	class SimpleWgSloImage : public QMainWindow
	{
		SLOImageWidget* imageWidget;
	public:
		explicit SimpleWgSloImage(QWidget* parent = nullptr) : QMainWindow(parent), imageWidget(new SLOImageWidget(this))
		{
			setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
			imageWidget->setImageSize(size());
			setCentralWidget(imageWidget);
		}

	protected:
		virtual void resizeEvent(QResizeEvent* event) override
		{
			imageWidget->setImageSize(event->size());
			QWidget::resizeEvent(event);
		}
	};

}


StupidSplineWindow::StupidSplineWindow(const char* filename)
: QMainWindow()
, dwSloImage         (new QDockWidget(this))
, bscanMarkerWidget  (new BScanMarkerWidget)
{
	ProgramOptions::bscansShowSegmentationslines.setValue(false);
	setMinimumWidth(1000);
// 	setMinimumHeight(600);

	bscanMarkerWidgetScrollArea = new ScrollAreaPan(this);
	bscanMarkerWidgetScrollArea->setWidget(bscanMarkerWidget);
	connect(bscanMarkerWidget, &CVImageWidget::needScrollTo, bscanMarkerWidgetScrollArea, &ScrollAreaPan::scrollTo);


	OctDataManager& octDataManager = OctDataManager::getInstance();
	connect(&octDataManager, &OctDataManager::loadFileSignal  , this, &StupidSplineWindow::loadFileStatusSlot);
	connect(&octDataManager, &OctDataManager::loadFileProgress, this, &StupidSplineWindow::loadFileProgress  );


	setWindowTitle(tr("OCT-Marker - simple spline gui"));


	QSettings& settings = ProgramOptions::getSettings();
	restoreGeometry(settings.value("stupidMainWindowGeometry").toByteArray());

	
	// General Objects
	setCentralWidget(bscanMarkerWidgetScrollArea);


	QHBoxLayout* layoutZoomControl = new QHBoxLayout;

	QSize buttonSize(50, 50);

	QToolButton* infoButton = new QToolButton(this);
	infoButton->setIcon(QIcon(":/icons/question_mark_1.svg"));
	infoButton->setIconSize(buttonSize);
	infoButton->setToolTip(tr("About"));
	connect(infoButton, &QToolButton::clicked, this, &StupidSplineWindow::showAboutDialog);
	layoutZoomControl->addWidget(infoButton);


	zoomInAction = new QAction(this);
	zoomInAction->setText(tr("Zoom +"));
	zoomInAction->setIcon(QIcon(":/icons/zoom_in_1.svg"));
	connect(zoomInAction, &QAction::triggered, bscanMarkerWidget, &CVImageWidget::zoom_in);
	QToolButton* buttonZoomIn = new QToolButton(this);
	buttonZoomIn->setDefaultAction(zoomInAction);
	buttonZoomIn->setIconSize(buttonSize);
	layoutZoomControl->addWidget(buttonZoomIn);

	zoomOutAction = new QAction(this);
	zoomOutAction->setText(tr("Zoom -"));
	zoomOutAction->setIcon(QIcon(":/icons/zoom_out_1.svg"));
	connect(zoomOutAction, &QAction::triggered, bscanMarkerWidget, &CVImageWidget::zoom_out);
	QToolButton* buttonZoomOut = new QToolButton(this);
	buttonZoomOut->setDefaultAction(zoomOutAction);
	buttonZoomOut->setIconSize(buttonSize);
	layoutZoomControl->addWidget(buttonZoomOut);


	QWidget* widgetZoomControl = new QWidget();
	widgetZoomControl->setLayout(layoutZoomControl);

	QDockWidget* dwZoomControl = new QDockWidget(this);
	dwZoomControl->setWindowTitle("Buttons");
	dwZoomControl->setWidget(widgetZoomControl);
	dwZoomControl->setFeatures(0);
	dwZoomControl->setObjectName("dwZoomControl");
	dwZoomControl->setTitleBarWidget(new QWidget());
	addDockWidget(Qt::LeftDockWidgetArea, dwZoomControl);



	SimpleWgSloImage* sloImageWidget = new SimpleWgSloImage(this);
	sloImageWidget->setMinimumWidth(250);
	sloImageWidget->setMinimumHeight(250);
	dwSloImage->setWindowTitle("SLO");
	dwSloImage->setWidget(sloImageWidget);
	dwSloImage->setFeatures(0);
	dwSloImage->setObjectName("StupidDWSloImage");
	dwSloImage->setTitleBarWidget(new QWidget());
	addDockWidget(Qt::LeftDockWidgetArea, dwSloImage);


	OctMarkerManager& marker = OctMarkerManager::getInstance();
	marker.setBscanMarkerTextID(QString("LayerSegmentation"));

	DWMarkerWidgets* dwmarkerwidgets = new DWMarkerWidgets(this);
	dwmarkerwidgets->setObjectName("DwMarkerWidgets");
	dwmarkerwidgets->setFeatures(0);
	dwmarkerwidgets->setTitleBarWidget(new QWidget());
	addDockWidget(Qt::LeftDockWidgetArea, dwmarkerwidgets);



	QDockWidget* dwSaveAndClose = new QDockWidget(this);
	dwSaveAndClose->setFeatures(0);
	dwSaveAndClose->setWindowTitle(tr("Quit"));
	dwSaveAndClose->setObjectName("dwSaveAndClose");
	QPushButton* buttonSaveAndClose = new QPushButton(this);
	buttonSaveAndClose->setText(tr("Save and Close"));
	buttonSaveAndClose->setFont(QFont("Times", 24, QFont::Bold));
	connect(buttonSaveAndClose, &QAbstractButton::clicked, this, &StupidSplineWindow::close);

	dwSaveAndClose->setWidget(buttonSaveAndClose);
	dwSaveAndClose->setTitleBarWidget(new QWidget());
	addDockWidget(Qt::LeftDockWidgetArea, dwSaveAndClose);



	// General Config
	setWindowIcon(QIcon(":/icons/image_edit.png"));

	connect(bscanMarkerWidget, &CVImageWidget::zoomChanged, this, &StupidSplineWindow::zoomChanged);


	if(filename)
		loadFile(filename);
}

StupidSplineWindow::~StupidSplineWindow()
{
}



void StupidSplineWindow::zoomChanged(double zoom)
{
	if(zoomInAction ) zoomInAction ->setEnabled(zoom < 5);
	if(zoomOutAction) zoomOutAction->setEnabled(zoom > 1);
}


BScanLayerSegmentation* StupidSplineWindow::getLayerSegmentationModul()
{
	OctMarkerManager& markerManager = OctMarkerManager::getInstance();
	BscanMarkerBase* markerModul = markerManager.getActBscanMarker();

	if(!markerModul)
		return nullptr;

	return dynamic_cast<BScanLayerSegmentation*>(markerModul);
}


bool StupidSplineWindow::saveLayerSegmentation()
{
	BScanLayerSegmentation* layerSegmentationModul = getLayerSegmentationModul();
	if(!layerSegmentationModul)
		return false;

	QString filename = OctDataManager::getInstance().getLoadedFilename() + ".segmentation.bin";
	return layerSegmentationModul->saveSegmentation2Bin(filename.toStdString());
}

bool StupidSplineWindow::copyLayerSegmentationFromOCTData()
{
	BScanLayerSegmentation* layerSegmentationModul = getLayerSegmentationModul();
	if(!layerSegmentationModul)
		return false;

	layerSegmentationModul->copyAllSegLinesFromOctData();
}




void StupidSplineWindow::closeEvent(QCloseEvent* e)
{
	if(!saveLayerSegmentation())
		QMessageBox::critical(this, tr("Error on save"), tr("Internal error in saveLayerSegmentation()"));


	// save programoptions
// 	ProgramOptions::loadOctdataAtStart.setValue(OctDataManager::getInstance().getLoadedFilename());
// 	ProgramOptions::writeAllOptions();

	QSettings& settings = ProgramOptions::getSettings();
	

	settings.setValue("stupidMainWindowGeometry", saveGeometry());
	settings.setValue("stupidMainWindowState"   , saveState()   );

	QWidget::closeEvent(e);
}




void StupidSplineWindow::loadFileStatusSlot(bool loading)
{
	if(loading)
	{
		if(!progressDialog)
			progressDialog = new QProgressDialog("Opening file ...", "Abort", 0, 100, this);

		progressDialog->setCancelButtonText(nullptr);
		progressDialog->setWindowModality(Qt::WindowModal);
		progressDialog->setValue(0);
		progressDialog->setVisible(true);
	}
	else
	{
		if(progressDialog)
		{
			progressDialog->setVisible(false);

			progressDialog->deleteLater();
			progressDialog = nullptr;
		}
		copyLayerSegmentationFromOCTData();
	}
}


void StupidSplineWindow::loadFileProgress(double frac)
{
	if(progressDialog)
		progressDialog->setValue(static_cast<int>(frac*100));
}



bool StupidSplineWindow::loadFile(const QString& filename)
{
	return OctFilesModel::getInstance().loadFile(filename);
}

void StupidSplineWindow::showAboutDialog()
{
	InfoDialogs::showAboutDialog(this);
}