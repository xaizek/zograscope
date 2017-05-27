// Copyright (C) 2016 xaizek <xaizek@openmailbox.org>
//
// This file is part of uncov.
//
// uncov is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// uncov is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with uncov.  If not, see <http://www.gnu.org/licenses/>.

#ifndef INTEGRATION_HPP__
#define INTEGRATION_HPP__

#include <memory>
#include <utility>

/**
 * @brief A class that automatically spawns pager if output is large.
 *
 * Output must come to @c std::cout and is considered to be large when it
 * doesn't fit screen height.
 */
class RedirectToPager
{
public:
    class Impl;

    /**
     * @brief Can redirect @c std::cout until destruction.
     */
    RedirectToPager();

    RedirectToPager(const RedirectToPager &rhs) = delete;
    RedirectToPager & operator=(const RedirectToPager &rhs) = delete;

    /**
     * @brief Restores previous state of @c std::cout.
     */
    ~RedirectToPager();

private:
    /**
     * @brief Implementation details.
     */
    std::unique_ptr<Impl> impl;
};

/**
 * @brief Queries whether program output is connected to terminal.
 *
 * @returns @c true if so, otherwise @c false.
 */
bool isOutputToTerminal();

/**
 * @brief Retrieves terminal width and height in characters.
 *
 * @returns Pair of actual terminal width and height, or maximum possible values
 *          of the type.
 */
std::pair<unsigned int, unsigned int> getTerminalSize();

#endif // INTEGRATION_HPP__
