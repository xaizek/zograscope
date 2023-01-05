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

#ifndef ZOGRASCOPE_TOOLS_TUI_VIEWMANAGER_HPP_
#define ZOGRASCOPE_TOOLS_TUI_VIEWMANAGER_HPP_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cursed/ColorTree.hpp"
#include "cursed/Placeholder.hpp"
#include "cursed/Track.hpp"

#include "vle/Mode.hpp"
#include "vle/Modes.hpp"

class FileRegistry;
class Language;
class Node;
class ViewManager;

struct ViewContext
{
    FileRegistry &registry;
    bool quit;
    bool viewChanged;
    const Language *lang;
    const Node *node;
};

class View
{
public:
    View(ViewManager &manager, std::string name);
    virtual ~View() = default;

public:
    const std::string & getName() const;
    const cursed::ColorTree & getHelpLine() const;
    const cursed::ColorTree & getStatusLine() const;
    cursed::Track & getTrack();

    virtual vle::Mode buildMode() = 0;
    virtual void update() = 0;

protected:
    cursed::ColorTree buildShortcut(const wchar_t label[],
                                    const wchar_t descr[]);
    void setStatusLine(cursed::ColorTree newStatusLine);

protected:
    ViewManager &manager;
    ViewContext &context;
    cursed::Track track;
    cursed::ColorTree helpLine;
    cursed::ColorTree statusLine;

    cursed::Format shortcutLabel;
    cursed::Format shortcutDescr;

private:
    std::string name;
};

class ViewManager
{
public:
    ViewManager(ViewContext &context, cursed::Placeholder &placeholder);

public:
    ViewContext & getContext();

    void push(const std::string &viewName);
    void pop();

    std::string getViewName();
    cursed::ColorTree getViewHelpLine();
    cursed::ColorTree getViewStatusLine();

private:
    void setupView(View &view);

private:
    ViewContext &context;
    cursed::Placeholder &placeholder;
    std::unordered_map<std::string, std::unique_ptr<View>> views;
    vle::Modes modes;
    std::vector<View *> stack;
};

#endif // ZOGRASCOPE_TOOLS_TUI_VIEWMANAGER_HPP_
