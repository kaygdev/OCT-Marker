/*
 * Copyright (c) 2018 Kay Gawlik <kaydev@amarunet.de> <kay.gawlik@beuth-hochschule.de> <kay.gawlik@charite.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once


#include<data_structure/point2d.h>
#include<data_structure/scalefactor.h>

#include<QPainter>


/**
 *  @ingroup FreeFormSegmentation
 *  @brief Base class for paint the free form segmentation
 *
 */
class PaintSegLine
{
public:
	virtual void paintLine(const Point2D& p1, const Point2D& p2) = 0;
};

/**
 *  @ingroup FreeFormSegmentation
 *  @brief Base class for paint with a QPen the free form segmentation
 *
 */
class PaintFactor : public PaintSegLine
{
protected:
	QPainter& painter;
public:
	PaintFactor(QPainter& painter) : painter(painter) {}

	void setPen(QPen& p) { painter.setPen(p); }
};


/**
 *  @ingroup FreeFormSegmentation
 *  @brief Paint the free from segmentation with QPainter when the scalefactor is identical
 *
 */
class PaintFactor1 : public PaintFactor
{
public:
	PaintFactor1(QPainter& painter) : PaintFactor(painter) {}

	void paintLine(const Point2D& p1, const Point2D& p2) override
	{
		painter.drawPoint(static_cast<int>(p1.getX()), static_cast<int>(p1.getY()));
		painter.drawPoint(static_cast<int>(p2.getX()), static_cast<int>(p2.getY()));
	}
};

/**
 *  @ingroup FreeFormSegmentation
 *  @brief Paint the free from segmentation with QPainter with a free scalefactor
 *
 */
class PaintFactorN : public PaintFactor
{
	ScaleFactor factor;
	const double factorX;
	const double factorY;

public:
	PaintFactorN(QPainter& painter, const ScaleFactor& factor) : PaintFactor(painter), factor(factor), factorX(factor.getFactorX()), factorY(factor.getFactorY()) {}


	void paintLine(const Point2D& p1, const Point2D& p2) override
	{
		painter.drawLine(static_cast<int>((p1.getX())*factorX + 0.5)
		               , static_cast<int>((p1.getY())*factorY + 0.5)
		               , static_cast<int>((p2.getX())*factorX + 0.5)
		               , static_cast<int>((p2.getY())*factorY + 0.5));
	}

};
