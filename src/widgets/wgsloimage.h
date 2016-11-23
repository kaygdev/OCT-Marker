#ifndef WGSLOIMAGE_H
#define WGSLOIMAGE_H

#include <QMainWindow>


class OctMarkerManager;
class SLOImageWidget;


class WgSloImage : public QMainWindow
{
	Q_OBJECT
	
	SLOImageWidget* imageWidget;
	OctMarkerManager& markerManger;
public:
	explicit WgSloImage(OctMarkerManager& markerManger, QWidget* parent = nullptr);
	
	
	virtual void resizeEvent(QResizeEvent* event);

protected:
	virtual void wheelEvent       (QWheelEvent*);
};

#endif // WGSLOIMAGE_H
