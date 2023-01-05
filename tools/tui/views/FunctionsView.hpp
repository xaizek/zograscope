// Copyright (C) 2019 xaizek <xaizek@posteo.net>
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

#ifndef ZOGRASCOPE_TOOLS_TUI_VIEWS_FUNCTIONSVIEW_HPP_
#define ZOGRASCOPE_TOOLS_TUI_VIEWS_FUNCTIONSVIEW_HPP_

#include <vector>

#include "cursed/Table.hpp"

#include "../ViewManager.hpp"

class FuncInfo;

class FunctionsView : public View
{
    enum class Sorting;

public:
    explicit FunctionsView(ViewManager &manager);

public:
    virtual vle::Mode buildMode() override;
    virtual void update() override;

private:
    void goToInfoMode(const std::string &mode);
    void setSorting(Sorting newSorting);
    void updateStatus();

private:
    cursed::Table table;
    std::vector<const FuncInfo *> sorted;
    Sorting sorting;
};

#endif // ZOGRASCOPE_TOOLS_TUI_VIEWS_FUNCTIONSVIEW_HPP_
