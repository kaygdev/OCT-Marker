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

#define _USE_MATH_DEFINES

#include "thicknessmap.h"

#include<map>
#include<limits>
#include<cmath>

#include<opencv2/opencv.hpp>

#include<octdata/datastruct/series.h>
#include<octdata/datastruct/bscan.h>
#include<octdata/datastruct/sloimage.h>

#include<helper/convertcolorspace.h>

#include<data_structure/matrx.h>
#include<data_structure/point2d.h>
#include<data_structure/programoptions.h>

#include"colormaphsv.h"


using Segmentline         = OctData::Segmentationlines::Segmentline;
using SegmentlineDataType = OctData::Segmentationlines::SegmentlineDataType;



ThicknessMap::ThicknessMap()
: thicknessMap(std::make_unique<cv::Mat>())
{
}



ThicknessMap::~ThicknessMap() = default;

void ThicknessMap::createMap(const SloBScanDistanceMap& distMap
                           , const std::vector<BScanLayerSegmentation::BScanSegData>& lines
                           , OctData::Segmentationlines::SegmentlineType t1
                           , OctData::Segmentationlines::SegmentlineType t2
                           , double scaleFactor
                           , const Colormap& colormap)
{
	const SloBScanDistanceMap::PreCalcDataMatrix* distMatrix = distMap.getDataMatrix();

	if(!distMatrix)
		return;

	bool blendColor = ProgramOptions::layerSegThicknessmapBlend();

	const std::size_t sizeX = distMatrix->getSizeX();
	const std::size_t sizeY = distMatrix->getSizeY();

	fillLineVec(lines, t1, t2);

	thicknessMap->create(static_cast<int>(sizeY), static_cast<int>(sizeX), CV_8UC4);

	for(std::size_t y = 0; y < sizeY; ++y)
	{
		uint8_t* destPtr = thicknessMap->ptr<uint8_t>(static_cast<int>(y));
		const SloBScanDistanceMap::PreCalcDataMatrix::value_type* srcPtr = distMatrix->scanLine(y);

		for(std::size_t x = 0; x < sizeX; ++x)
		{
			if(srcPtr->init)
			{
				double value;
				if(blendColor) value = getMixValue(*srcPtr);
				else           value = getSingleValue(*srcPtr);

				if(value < 0.)
				{
					destPtr[0] = 0;
					destPtr[1] = 0;
					destPtr[2] = 0;
					destPtr[3] = 0;
				}
				else
				{
					double mixThickness = value*scaleFactor;

					colormap.getColor(mixThickness, destPtr[2], destPtr[1], destPtr[0]);
					destPtr[3] = 255;
				}

			}
			else
			{
				destPtr[0] = 0;
				destPtr[1] = 0;
				destPtr[2] = 0;
				destPtr[3] = 0;
			}

			destPtr += 4;
			++srcPtr;
		}

	}
}

double ThicknessMap::getSingleValue(const SloBScanDistanceMap::PixelInfo& pinfo) const
{
	const SloBScanDistanceMap::InfoBScanDist& bInfo1 = pinfo.bscan1;
	const double height = getValue(bInfo1);
	if(std::isnan(height))
		return -1;
	return height;
}


double ThicknessMap::getMixValue(const SloBScanDistanceMap::PixelInfo& pinfo) const
{
	const SloBScanDistanceMap::InfoBScanDist& bInfo1 = pinfo.bscan1;
	const SloBScanDistanceMap::InfoBScanDist& bInfo2 = pinfo.bscan2;

	const double h1 = getValue(bInfo1);
	if(std::isnan(h1) || h1 < 0)
		return -1;

	if(bInfo1.distance == 0)
		return h1;

	const double h2 = getValue(bInfo2);
	if(std::isnan(h2) || h2 < 0)
		return h1;

	const double l = bInfo1.distance + bInfo2.distance;
	if(l == 0)
		return h1;

	const double w1 = bInfo2.distance/l;
	const double w2 = bInfo1.distance/l;

	const double result = h1*w1 + h2*w2;

	return result;
}


inline double ThicknessMap::getValue(const SloBScanDistanceMap::InfoBScanDist& info) const
{
	const std::size_t ascan = info.ascan;
	const std::size_t bscan = info.bscan;

	if(ascan >= thicknessMatrix.getSizeX() || bscan >= thicknessMatrix.getSizeY())
		return std::numeric_limits<double>::quiet_NaN();

	return thicknessMatrix(ascan, bscan);
}



void ThicknessMap::initThicknessMatrix(const std::vector<BScanLayerSegmentation::BScanSegData>& lines, OctData::Segmentationlines::SegmentlineType t1, OctData::Segmentationlines::SegmentlineType t2)
{
	const std::size_t numBscans = lines.size();
	std::size_t maxAscanNum = 0;

	for(const BScanLayerSegmentation::BScanSegData& segData : lines)
	{
		const OctData::Segmentationlines::Segmentline* l1 = nullptr;
		const OctData::Segmentationlines::Segmentline* l2 = nullptr;
		if(segData.filled)
		{
			l1 = &(segData.lines.getSegmentLine(t1));
			l2 = &(segData.lines.getSegmentLine(t2));

			const std::size_t numAscans = std::min(l1->size(), l2->size());
			if(numAscans > maxAscanNum)
				maxAscanNum = numAscans;
		}
	}

	thicknessMatrix.resize(maxAscanNum, numBscans);
}

void ThicknessMap::fillLineVec(const std::vector<BScanLayerSegmentation::BScanSegData>& lines
                             , OctData::Segmentationlines::SegmentlineType t1
                             , OctData::Segmentationlines::SegmentlineType t2)
{
	initThicknessMatrix(lines, t1, t2);
	std::size_t nrBscan = 0;
	for(const BScanLayerSegmentation::BScanSegData& segData : lines)
	{
		fillThicknessBscan(segData, nrBscan, t1, t2);
		++nrBscan;
	}
}

namespace
{
	double getValue(const double* const line, std::size_t index)
	{
		const double value = line[index];
		if(value > 1e8)
			return std::numeric_limits<double>::quiet_NaN();
		return value;
	}
}

void ThicknessMap::fillThicknessBscan(const BScanLayerSegmentation::BScanSegData& bscanData, const std::size_t bscanNr, OctData::Segmentationlines::SegmentlineType t1, OctData::Segmentationlines::SegmentlineType t2)
{
	double* const scanline = thicknessMatrix.scanLine(bscanNr);
	const std::size_t maxAscanNum = thicknessMatrix.getSizeX();

	std::size_t filledAscans = 0;
	if(bscanData.filled)
	{
		const OctData::Segmentationlines::Segmentline* l1 = &(bscanData.lines.getSegmentLine(t1));
		const OctData::Segmentationlines::Segmentline* l2 = &(bscanData.lines.getSegmentLine(t2));
		if(l1 && l2)
		{
			const std::size_t numAscans = std::min(std::min(l1->size(), l2->size()), maxAscanNum);

			const double* const l1data = l1->data();
			const double* const l2data = l2->data();

			for(std::size_t i = 0; i < numAscans; ++i)
			{
				const double v1 = ::getValue(l1data, i);
				const double v2 = ::getValue(l2data, i);
				if(std::isnan(v1) || std::isnan(v2))
					scanline[i] = std::numeric_limits<double>::quiet_NaN();
				else
					scanline[i] = v2 - v1;
			}
			filledAscans = numAscans;
		}
	}
	for(std::size_t i = filledAscans; i < maxAscanNum; ++i)
		scanline[i] = std::numeric_limits<double>::quiet_NaN();
}


void ThicknessMap::resetThicknessMapCache()
{
}
