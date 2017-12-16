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

#include "modules/accessibility/AXARIAGridRow.h"

#include "modules/accessibility/AXARIAGridCell.h"
#include "modules/accessibility/AXObjectCacheImpl.h"
#include "modules/accessibility/AXTable.h"
#include "modules/accessibility/AXTableCell.h"


namespace blink {

AXARIAGridRow::AXARIAGridRow(LayoutObject* layoutObject, AXObjectCacheImpl& axObjectCache)
    : AXTableRow(layoutObject, axObjectCache)
{
}

AXARIAGridRow::~AXARIAGridRow()
{
}

AXARIAGridRow* AXARIAGridRow::create(LayoutObject* layoutObject, AXObjectCacheImpl& axObjectCache)
{
    return new AXARIAGridRow(layoutObject, axObjectCache);
}

bool AXARIAGridRow::isARIATreeGridRow() const
{
    AXObject* parent = parentTable();
    if (!parent)
        return false;

    return parent->ariaRoleAttribute() == TreeGridRole;
}

void AXARIAGridRow::headerObjectsForRow(AXObjectVector& headers)
{
    for (const auto& cell : cells()) {
        if (cell->roleValue() == RowHeaderRole)
            headers.append(cell);
    }
}

AXObject* AXARIAGridRow::parentTable() const {
  for (AXObject* parent = parentObjectUnignored(); parent;
       parent = parent->parentObjectUnignored()) {
    if (parent->isAXTable() && toAXTable(parent)->isAriaTable())
      return parent;
  }
  return nullptr;
}

void AXARIAGridRow::addChildren() {
  AXLayoutObject::addChildren();

  if (!isTableRow())
    return;

  HeapHashSet<Member<AXObject>> appendedCells;
  for (const auto& child : children()) {
    traverseChildrenForCells(child, appendedCells);
  }
}

void AXARIAGridRow::clearChildren() {
  AXLayoutObject::clearChildren();
  m_cells.clear();
}

void AXARIAGridRow::traverseChildrenForCells(
    AXObject* child, HeapHashSet<Member<AXObject>>& appendedCells) {
  if (appendedCells.contains(child))
    return;

  if (!child->isTableCell()) {
    for (const auto& grandchild : child->children())
      traverseChildrenForCells(grandchild, appendedCells);
    return;
  }

  AXTableCell* cell = toAXTableCell(child);
  if (cell->isAriaCell()) {
    static_cast<AXARIAGridCell*>(cell)->setCellIndex(
        rowIndex(), m_cells.size());
  }
  m_cells.append(cell);
  appendedCells.add(cell);
}

const AXLayoutObject::AXObjectVector& AXARIAGridRow::cells() {
  updateChildrenIfNecessary();

  return m_cells;
}

DEFINE_TRACE(AXARIAGridRow) {
  visitor->trace(m_cells);
  AXTableRow::trace(visitor);
}

} // namespace blink
