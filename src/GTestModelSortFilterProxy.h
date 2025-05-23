//--------------------------------------------------------------------------------------------------
// 
///	@PROJECT	gtest-runner
/// @BRIEF		Filter proxy that does sorting on tree models
///	@DETAILS	
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
// Copyright (c) 2016 Nic Holthaus
// Copyright (c) 2024 Oliver Karrenbauer
// 
//--------------------------------------------------------------------------------------------------

#pragma once

//------------------------
//	INCLUDES
//------------------------
#include "flatDomeItem.h"
#include <QSortFilterProxyModel>

//	--------------------------------------------------------------------------------
///	@class		GTestModelSortFilterProxy
///	@brief		Filter Proxy Model which searches from the bottom up.
///	@details	Unlike the traditional QSortFilterProxyModel, this model will match
///				children, and show them and their parents if they match the filter.
///				Thus, for a tree view, if any node in the hierarchy matches the regex,
///				the entire branch it lives in will be shown.
//  --------------------------------------------------------------------------------
class GTestModelSortFilterProxy : public QSortFilterProxyModel
{
	Q_OBJECT

public:
	explicit GTestModelSortFilterProxy(QObject* parent = nullptr);

	void setShowNotExecuted(const bool value)
	{
		showNotExecuted_ = value;
		invalidateFilter();
	}

	void setShowPassed(const bool value)
	{
		showPassed_ = value;
		invalidateFilter();
	}

	void setShowIgnored(const bool value)
	{
		showIgnored_ = value;
		invalidateFilter();
	}

protected:
	bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
	bool filterAcceptsDescendant(int sourceRow) const;

	bool checkShowItem(const FlatDomeItemPtr& resultItem, const QModelIndex& resultRowToTest, int level) const;

	bool filterAcceptsAncestor(int sourceRow) const;

	bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

private:
	bool showNotExecuted_ = true;
	bool showPassed_ = true;
	bool showIgnored_ = true;
};
