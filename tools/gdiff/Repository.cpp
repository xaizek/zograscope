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

#include "Repository.hpp"

#include <git2.h>

#include <boost/scope_exit.hpp>

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "DiffList.hpp"

// A RAII wrapper that manages lifetime of libgit2's handles.
template <typename T>
class Repository::GitObjPtr
{
public:
    // Frees the handle.
    ~GitObjPtr()
    {
        if (ptr != nullptr) {
            deleteObj(ptr);
        }
    }

public:
    // Implicitly converts to pointer value.
    operator T*()
    {
        return ptr;
    }

    // Convertion to a pointer to allow external initialization.
    T ** operator &()
    {
        return &ptr;
    }

    // Pointer convertion.
    template <typename U>
    U * as()
    {
        return reinterpret_cast<U *>(ptr);
    }

private:
    // Frees `git_object`.
    void deleteObj(git_object *ptr)
    {
        git_object_free(ptr);
    }

    // Frees `git_status_list`.
    void deleteObj(git_status_list *ptr)
    {
        git_status_list_free(ptr);
    }

    // Frees `git_blob`.
    void deleteObj(git_blob *ptr)
    {
        git_blob_free(ptr);
    }

    // Catches implicit conversions and unhandled cases at compile-time.
    void deleteObj(void *ptr) = delete;

private:
    T *ptr = nullptr; // Wrapped pointer.
};

LibGitUser::LibGitUser()
{
    git_libgit2_init();
}

LibGitUser::~LibGitUser()
{
    git_libgit2_shutdown();
}

Repository::Repository(const std::string &path)
{
    git_buf repoPath = GIT_BUF_INIT_CONST(NULL, 0);
    if (git_repository_discover(&repoPath, path.c_str(), false, nullptr) != 0) {
        throw std::invalid_argument("Could not discover repository");
    }
    BOOST_SCOPE_EXIT_ALL(&repoPath) { git_buf_free(&repoPath); };

    if (git_repository_open(&repo, repoPath.ptr) != 0) {
        throw std::invalid_argument("Could not open repository");
    }
}

Repository::~Repository()
{
    git_repository_free(repo);
}

std::vector<DiffEntry>
Repository::listStatus(bool staged)
{
    git_status_options statusOpts = GIT_STATUS_OPTIONS_INIT;
    statusOpts.show = staged ? GIT_STATUS_SHOW_INDEX_ONLY
                             : GIT_STATUS_SHOW_WORKDIR_ONLY;
    GitObjPtr<git_status_list> statusList;
    if (git_status_list_new(&statusList, repo, &statusOpts) != 0) {
        throw std::runtime_error("Failed to list status of files");
    }

    auto readObj = [&](const git_oid &id) {
        GitObjPtr<git_blob> blob;
        if (git_blob_lookup(&blob, repo, &id) != 0) {
            throw std::runtime_error("Failed to read a blob");
        }
        return std::string(static_cast<const char *>(git_blob_rawcontent(blob)),
                           static_cast<std::size_t>(git_blob_rawsize(blob)));
    };

    std::string workdirPath = git_repository_workdir(repo);
    auto makeEntry = [&](git_diff_file &file, bool forceRead) {
        if (git_oid_iszero(&file.id) || forceRead) {
            return DiffEntryFile(file.path, workdirPath + '/' + file.path);
        }
        return DiffEntryFile(file.path, file.path, readObj(file.id));
    };

    std::vector<DiffEntry> entries;
    for (size_t i = 0;
         const git_status_entry *entry = git_status_byindex(statusList, i);
         ++i) {
        git_diff_delta *delta = entry->head_to_index != nullptr
                              ? entry->head_to_index
                              : entry->index_to_workdir;

        if (delta->status == GIT_DELTA_MODIFIED) {
            entries.push_back({ makeEntry(delta->old_file, false),
                                makeEntry(delta->new_file, !staged) });
        }
    }

    return entries;
}
