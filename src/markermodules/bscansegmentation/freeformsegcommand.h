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

#ifndef FREEFORMSEGCOMMAND_H
#define FREEFORMSEGCOMMAND_H

#include<cstddef>

#include<markermodules/markercommand.h>

class BScanSegmentation;
class SimpleCvMatCompress;

/**
 *  @ingroup FreeFormSegmentation
 *  @brief Class for supporting undo and redo function of the free form segmentation
 *
 */
class FreeFormSegCommand : public MarkerCommand
{
	BScanSegmentation& parent;
	SimpleCvMatCompress* segmentationMat;

	std::size_t bscanNr;

public:
	FreeFormSegCommand(BScanSegmentation& parent, const SimpleCvMatCompress& mat);
	~FreeFormSegCommand() override;

	FreeFormSegCommand(const FreeFormSegCommand &other)            = delete;
	FreeFormSegCommand &operator=(const FreeFormSegCommand &other) = delete;


	void apply() override;
	bool undo() override;
	bool redo() override;
};

#endif // FREEFORMSEGCOMMAND_H
