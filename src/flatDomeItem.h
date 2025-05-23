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

#pragma once

#include <QDomNode>
#include <vector>
#include <functional>
#include <memory>


class FlatDomeItem
{
public:
    explicit FlatDomeItem(const QDomNode& node, int level, int row, int parentIndex);

    const QDomNode& node() const;

    int level() const;

    int row() const;

    int parentIndex() const;

    const std::vector<int>& childrenIndex() const
    {
        return childrenIndex_;
    }

    void addChildIndex(int childIndex)
    {
        childrenIndex_.emplace_back(childIndex);
    }

private:
    QDomNode node_;
    int level_;
    int row_;
    int parentIndex_;
    std::vector<int> childrenIndex_;
};

using FlatDomeItemPtr = std::shared_ptr<FlatDomeItem>;


class FlatDomeItemHandler
{
public:
    using FilterFunc = std::function<bool(const QDomNode&)>;

    explicit FlatDomeItemHandler(const QDomNode& rootNode, const QDomNode& rootReferenceNode, FilterFunc filterFunc);

    FlatDomeItemPtr item(std::size_t row) const;

    int numberItems() const;

private:
    void addChildren(const QDomNode& node, const QDomNode& referenceNode, int level, int& row, int parentIndex);

    void addItem(const QDomNode& node, int level, int& row, int parentIndex);

    void addEmptyItem(int level, int& row);

    bool shouldAddItem(const QDomNode& node) const;


    std::vector<FlatDomeItemPtr> items_;
    FilterFunc filterFunc_;
};
