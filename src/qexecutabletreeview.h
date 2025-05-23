//--------------------------------------------------------------------------------------------------
// 
///	@PROJECT	gtest-runner
/// @BRIEF		Tree view subclass which allows the creation/destruction of the menu widget when rows are added
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
// Copyright (c) 2020 Oliver Karrenbauer
// 
//--------------------------------------------------------------------------------------------------

#pragma once

//------------------------------
//	INCLUDE
//------------------------------
#include <QTreeView>

//------------------------------
//	FORWARD DECLARATIONS
//------------------------------

class QExecutableTreeViewPrivate;

//--------------------------------------------------------------------------------------------------
//	CLASS: QExecutableTreeView
//--------------------------------------------------------------------------------------------------
/// @brief		
/// @details	
//--------------------------------------------------------------------------------------------------
class QExecutableTreeView final : public QTreeView
{
    Q_OBJECT
public:

	explicit QExecutableTreeView(QWidget* parent = nullptr);

    signals:
        void itemSelectionChanged();

protected:
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;

    bool eventFilter(QObject* obj, QEvent* event) override;

    void rowsInserted(const QModelIndex &parent, int start, int end) override;

    void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end) override;

private:

	Q_DECLARE_PRIVATE(QExecutableTreeView);
	QExecutableTreeViewPrivate*	d_ptr;
};	// CLASS: QExecutableTreeView

