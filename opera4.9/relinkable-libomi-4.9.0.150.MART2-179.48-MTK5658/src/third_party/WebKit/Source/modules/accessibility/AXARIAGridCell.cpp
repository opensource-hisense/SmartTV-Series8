/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "modules/accessibility/AXARIAGridCell.h"

#include "modules/accessibility/AXObjectCacheImpl.h"
#include "modules/accessibility/AXTable.h"
#include "modules/accessibility/AXTableRow.h"


namespace blink {

AXARIAGridCell::AXARIAGridCell(LayoutObject* layoutObject, AXObjectCacheImpl& axObjectCache)
    : AXTableCell(layoutObject, axObjectCache)
{
}

AXARIAGridCell::~AXARIAGridCell()
{
}

AXARIAGridCell* AXARIAGridCell::create(LayoutObject* layoutObject, AXObjectCacheImpl& axObjectCache)
{
    return new AXARIAGridCell(layoutObject, axObjectCache);
}

bool AXARIAGridCell::isAriaColumnHeader() const
{
    const AtomicString& role = getAttribute(HTMLNames::roleAttr);
    return equalIgnoringCase(role, "columnheader");
}

bool AXARIAGridCell::isAriaRowHeader() const
{
    const AtomicString& role = getAttribute(HTMLNames::roleAttr);
    return equalIgnoringCase(role, "rowheader");
}

AXObject* AXARIAGridCell::parentTable() const
{
  for (AXObject* parent = parentObjectUnignored(); parent;
       parent = parent->parentObjectUnignored()) {
    if (parent->isAXTable() && toAXTable(parent)->isAriaTable())
      return parent;
  }
  return nullptr;
}

void AXARIAGridCell::setCellIndex(unsigned rowIndex, unsigned columnIndex)
{
  m_rowIndex = rowIndex;
  m_columnIndex = columnIndex;
}

void AXARIAGridCell::rowIndexRange(std::pair<unsigned, unsigned>& rowRange)
{
  // Aria grid cell cannot have span rows.
  rowRange = { m_rowIndex, 1 };
}

void AXARIAGridCell::columnIndexRange(
    std::pair<unsigned, unsigned>& columnRange)
{
  // Aria grid cell cannot have span columns.
  columnRange = { m_columnIndex, 1 };
}

AccessibilityRole AXARIAGridCell::scanToDecideHeaderRole()
{
    if (isAriaRowHeader())
        return RowHeaderRole;

    if (isAriaColumnHeader())
        return ColumnHeaderRole;

    return CellRole;
}

} // namespace blink
