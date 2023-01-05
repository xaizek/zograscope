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

#ifndef ZOGRASCOPE_TOOLS_GDIFF_REPOSITORY_HPP_
#define ZOGRASCOPE_TOOLS_GDIFF_REPOSITORY_HPP_

#include <string>
#include <vector>

// This unit provides facilities for interacting with a repository.  With git
// being the only VCS that is supported.

struct git_repository;

class DiffEntry;

// Simple RAII class to keep track of libgit2 usage.
class LibGitUser
{
public:
    // Informs libgit2 about one more client.
    LibGitUser();
    // No copy/move constructing.
    LibGitUser(const LibGitUser &rhs) = delete;
    // No copy/move assigning.
    LibGitUser & operator=(const LibGitUser &rhs) = delete;
    // Informs libgit2 about one less client.
    ~LibGitUser();
};

// Provides high-level access to repository data.
class Repository
{
public:
    // Creates an instance from path to or in repository.  Throws
    // `std::runtime_error` on failure to find or open repository.
    explicit Repository(const std::string &path);

    // No copy/move constructing.
    Repository(const Repository &rhs) = delete;
    // No copy/move assigning.
    Repository & operator=(const Repository &rhs) = delete;

    // Frees resources allocated for the repository.
    ~Repository();

public:
    // Lists either staged or unstaged modified entries in the working directory
    // of the repository.
    std::vector<DiffEntry> listStatus(bool staged);

    // Lists change set of specified reference against its parent.
    std::vector<DiffEntry> listCommit(const std::string &ref);

public:
    const LibGitUser libgitUser; // libgit2 lifetime management.
    git_repository *repo;        // Repository handle.
};

#endif // ZOGRASCOPE_TOOLS_GDIFF_REPOSITORY_HPP_
