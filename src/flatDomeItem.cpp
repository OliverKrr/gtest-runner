//--------------------------------------------------------------------------------------------------
//
///	@PROJECT	project
/// @BRIEF		brief
///	@DETAILS	details
//
//--------------------------------------------------------------------------------------------------
//
// The MIT License (MIT)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software
// and associated documentation files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
// BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//--------------------------------------------------------------------------------------------------
//
// ATTRIBUTION:
// Parts of this work have been adapted from:
//
//--------------------------------------------------------------------------------------------------
//
// Copyright (c) 2021 Oliver Karrenbauer
//
//--------------------------------------------------------------------------------------------------

#include "flatDomeItem.h"

#include <utility>


FlatDomeItem::FlatDomeItem(const QDomNode& node, const int level, const int row, const int parentIndex)
    : node_(node), level_(level), row_(row), parentIndex_(parentIndex)
{
    // nothing
}

const QDomNode& FlatDomeItem::node() const
{
    return node_;
}

int FlatDomeItem::level() const
{
    return level_;
}

int FlatDomeItem::row() const
{
    return row_;
}

int FlatDomeItem::parentIndex() const
{
    return parentIndex_;
}


FlatDomeItemHandler::FlatDomeItemHandler(const QDomNode& rootNode, const QDomNode& rootReferenceNode,
                                         FilterFunc filterFunc)
    : filterFunc_(std::move(filterFunc))
{
    if (rootNode.isNull() || rootReferenceNode.isNull())
    {
        return;
    }
    constexpr int level = 0;
    int row = 0;
    addChildren(rootNode, rootReferenceNode, level, row, -1);
}

void FlatDomeItemHandler::addChildren(const QDomNode& node, const QDomNode& referenceNode,
                                      const int level, int& row, const int parentIndex)
{
    int currentOffset = 0;
    for (int i = 0; i < referenceNode.childNodes().count(); ++i)
    {
        const auto& referenceChildNode = referenceNode.childNodes().item(i);
        if (!shouldAddItem(referenceChildNode))
        {
            continue;
        }

        QString referenceChildName = referenceChildNode.attributes().namedItem("name").nodeValue();
        bool found = false;
        for (int j = currentOffset; j < node.childNodes().count(); ++j)
        {
            // Search if we contain node from reference
            const auto& childNode = node.childNodes().item(j);
            QString childName = childNode.attributes().namedItem("name").nodeValue();
            if (referenceChildName == childName)
            {
                found = true;
                currentOffset = j;
                break;
            }
        }
        // Follow reference recursively
        QDomNode childNode;
        if (found)
        {
            childNode = node.childNodes().item(currentOffset);
            addItem(childNode, level, row, parentIndex);
        }
        else
        {
            // keep childNode -> still need to follow referenceChildNode
            addEmptyItem(level, row);
        }

        addChildren(childNode, referenceChildNode, level + 1, row, static_cast<int>(items_.size() - 1));
    }
}

void FlatDomeItemHandler::addItem(const QDomNode& node, const int level, int& row, const int parentIndex)
{
    int itemRow = level == 2 ? ++row : 0;
    auto item = std::make_shared<FlatDomeItem>(node, level, itemRow, parentIndex);
    items_.emplace_back(item);
    if (parentIndex >= 0)
    {
        items_[static_cast<size_t>(parentIndex)]->addChildIndex(static_cast<int>(items_.size() - 1));
    }
}

void FlatDomeItemHandler::addEmptyItem(const int level, int& row)
{
    if (level == 2)
    {
        ++row;
    }
    items_.emplace_back(nullptr);
}

bool FlatDomeItemHandler::shouldAddItem(const QDomNode& node) const
{
    return !filterFunc_ || filterFunc_(node);
}

FlatDomeItemPtr FlatDomeItemHandler::item(const std::size_t row) const
{
    if (row < items_.size())
    {
        return items_[row];
    }
    return nullptr;
}

int FlatDomeItemHandler::numberItems() const
{
    return static_cast<int>(items_.size());
}
