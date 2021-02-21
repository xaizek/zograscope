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

#include "ViewManager.hpp"

#include <algorithm>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "cursed/Placeholder.hpp"

#include "views/CodeView.hpp"
#include "views/DumpView.hpp"
#include "views/FilesView.hpp"
#include "views/FunctionsView.hpp"

View::View(ViewManager &manager, std::string name)
    : manager(manager), context(manager.getContext()), name(std::move(name))
{
    shortcutLabel.setForeground(cursed::Color::Yellow);
    shortcutLabel.setReversed(true);
    shortcutLabel.setBold(true);
}

const std::string &
View::getName() const
{
    return name;
}

const cursed::ColorTree &
View::getHelpLine() const
{
    return helpLine;
}

const cursed::ColorTree &
View::getStatusLine() const
{
    return statusLine;
}

cursed::Track &
View::getTrack()
{
    return track;
}

cursed::ColorTree
View::buildShortcut(const wchar_t label[], const wchar_t descr[])
{
    return shortcutLabel(L" ") + shortcutLabel(label) + shortcutLabel(L" ")
         + L" " + shortcutDescr(descr) + L" ";
}

void
View::setStatusLine(cursed::ColorTree newStatusLine)
{
    statusLine = std::move(newStatusLine);
}

ViewManager::ViewManager(ViewContext &context, cursed::Placeholder &placeholder)
    : context(context), placeholder(placeholder)
{
    auto filesView = std::unique_ptr<View>(new FilesView(*this));
    views.emplace(filesView->getName(), std::move(filesView));
    auto functionsView = std::unique_ptr<View>(new FunctionsView(*this));
    views.emplace(functionsView->getName(), std::move(functionsView));
    auto dumpView = std::unique_ptr<View>(new DumpView(*this));
    views.emplace(dumpView->getName(), std::move(dumpView));
    auto codeView = std::unique_ptr<View>(new CodeView(*this));
    views.emplace(codeView->getName(), std::move(codeView));

    std::vector<vle::Mode> allModes;
    for (const auto &entry : views) {
        allModes.emplace_back(entry.second->buildMode());
    }
    modes.setModes(std::move(allModes));
}

ViewContext &
ViewManager::getContext()
{
    return context;
}

void
ViewManager::push(const std::string &viewName)
{
    View &view = *views.at(viewName);

    auto it = std::find(stack.begin(), stack.end(), &view);
    if (it == stack.end()) {
        stack.push_back(&view);
        view.update();
    } else {
        std::rotate(it, std::next(it), stack.end());
    }

    setupView(view);
}

void
ViewManager::pop()
{
    if (stack.empty()) {
        throw std::runtime_error("Can't pop a view from an empty list of "
                                 "views");
    }
    stack.pop_back();

    if (!stack.empty()) {
        setupView(*stack.back());
    }
}

void
ViewManager::setupView(View &view)
{
    modes.switchTo(view.getName());
    placeholder.fill(&view.getTrack());
    context.viewChanged = true;
}

std::string
ViewManager::getViewName()
{
    return (stack.empty() ? std::string() : stack.back()->getName());
}

cursed::ColorTree
ViewManager::getViewHelpLine()
{
    return stack.empty()
         ? cursed::ColorTree()
         : L" " + cursed::ColorTree(stack.back()->getHelpLine());
}

cursed::ColorTree
ViewManager::getViewStatusLine()
{
    return stack.empty()
         ? cursed::ColorTree()
         : stack.back()->getStatusLine();
}
