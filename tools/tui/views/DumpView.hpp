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

#ifndef ZOGRASCOPE_TOOLS_TUI_VIEWS_DUMPVIEW_HPP_
#define ZOGRASCOPE_TOOLS_TUI_VIEWS_DUMPVIEW_HPP_

#include "cursed/Text.hpp"

#include "../ViewManager.hpp"

class DumpView : public View
{
public:
    explicit DumpView(ViewManager &manager);

public:
    virtual vle::Mode buildMode() override;
    virtual void update() override;

private:
    cursed::Text text;
};

#endif // ZOGRASCOPE_TOOLS_TUI_VIEWS_DUMPVIEW_HPP_
