// Copyright (C) 2018 xaizek <xaizek@posteo.net>
//
// This file is part of zograscope.
//
// zograscope is free software: you can redistribute it and/or modify
// it under the terms of version 3 of the GNU Affero General Public License as
// published by the Free Software Foundation.
//
// zograscope is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with zograscope.  If not, see <http://www.gnu.org/licenses/>.

#ifndef ZOGRASCOPE_TOOLING_FUNCTIONANALYZER_HPP_
#define ZOGRASCOPE_TOOLING_FUNCTIONANALYZER_HPP_

class Language;
class Node;

// Computes properties of functions.
class FunctionAnalyzer
{
public:
    // Remembers language, which will be used to analyze nodes.
    explicit FunctionAnalyzer(Language &lang);

public:
    // Retrieves number of lines taken by the function (includes all of its
    // elements).
    int getLineCount(const Node *node) const;
    // Retrieves number of parameters that the function accepts.
    int getParamCount(const Node *node) const;

private:
    const Language &lang; // Language of the functions being analyzed.
};

#endif // ZOGRASCOPE_TOOLING_FUNCTIONANALYZER_HPP_
