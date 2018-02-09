// Copyright (C) 2018 xaizek <xaizek@posteo.net>
//
// This file is part of zograscope.
//
// zograscope is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// zograscope is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with zograscope.  If not, see <http://www.gnu.org/licenses/>.

#include "utils/time.hpp"

#include <boost/scope_exit.hpp>

#include <iomanip>
#include <ostream>

std::ostream &
operator<<(std::ostream &os, const TimeReport &tr)
{
    using Measure = TimeReport::Measure;

    struct MeasureTraits
    {
        static unsigned int size(const Measure *node)
        {
            return node->children.size();
        }

        static const Measure * getChild(const Measure *node, unsigned int i)
        {
            return &node->children[i];
        }
    };

    using msf = std::chrono::duration<float, std::milli>;

    auto osState = os.rdstate();
    BOOST_SCOPE_EXIT_ALL(&os, &osState) { os.setstate(osState); };

    os << std::fixed << std::setprecision(3);

    trees::printSetTraits<MeasureTraits>(os, &tr.root,
                 [](std::ostream &os, const Measure *m) {
                     msf duration = m->end - m->start;
                     os << m->stage << " -- " << duration.count() << "ms";

                     if (m->children.empty()) {
                         os << '\n';
                         return;
                     }

                     msf accounted = {};
                     for (const Measure &child : m->children) {
                         accounted += child.end - child.start;
                     }
                     msf unaccounted = duration - accounted;
                     os << " (-" << unaccounted.count() << "ms)\n";
                 });

    return os;
}
