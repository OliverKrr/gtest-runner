//--------------------------------------------------------------------------------------------------
// 
///	@PROJECT	gtest-runner
/// @BRIEF		GTest std::out syntax highlighter
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
#include <QSyntaxHighlighter>
#include <QColor>
#include <QTextEdit>
#include <QRegularExpression>

//	----------------------------------------------------------------------------
//	CLASS		QStdOutSyntaxHighlighter
//  ----------------------------------------------------------------------------
///	@brief		provides syntax highlighting for the std::out console.
///	@details	
//  ----------------------------------------------------------------------------
class QStdOutSyntaxHighlighter : public QSyntaxHighlighter
{
	enum blockState
	{

	};

public:

	explicit QStdOutSyntaxHighlighter(QTextEdit* parent) : QSyntaxHighlighter(parent)
	{
		HighlightingRule rule;

 		blockFormat.setForeground(QColor("#00ff00"));
 		rule.pattern = QRegularExpression(R"(\[((?!\s+DEATH\s+).)*\])");
 		rule.format = blockFormat;
		highlightingRules.append(rule);

		errorFormat.setForeground(QColor("#ff0000"));
		rule.pattern = QRegularExpression("\\[.*FAILED.*\\]");
		rule.format = errorFormat;
		highlightingRules.append(rule);

		errorFormat.setForeground(QColor("#ffd700"));
		rule.pattern = QRegularExpression("TEST RUN .*");
		rule.format = errorFormat;
		highlightingRules.append(rule);
	}

	void highlightBlock(const QString &text) override
	{
		foreach(const HighlightingRule &rule, highlightingRules) {
			QRegularExpression expression(rule.pattern);
			auto i = expression.globalMatch(text);
			while (i.hasNext()) {
				const auto match = i.next();
				const int index = match.capturedStart();
				const int length = match.capturedLength();
				setFormat(index, length, rule.format);
			}
		}
	}

private:

	struct HighlightingRule
	{
		QRegularExpression		pattern;
		QTextCharFormat			format;
	};

	QVector<HighlightingRule>	highlightingRules;

	QTextCharFormat				errorFormat;													///< Highlight style for errors.
	QTextCharFormat				blockFormat;													///< Highlight style for network connection related messages.
	QTextCharFormat				timestampFormat;												///< highlight style for timestamps
};
