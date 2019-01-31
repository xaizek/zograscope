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

namespace {

class GitException : public std::runtime_error
{
public:
    GitException(const std::string &msg)
        : std::runtime_error(addMoreInfo(msg))
    { }

private:
    static std::string addMoreInfo(const std::string &msg)
    {
        if (const git_error *err = giterr_last()) {
            return msg + " (" + err->message + ')';
        }
        return msg;
    }
};

// A RAII wrapper that manages lifetime of libgit2's handles.
template <typename T>
class GitObjPtr
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

    // Frees `git_diff`.
    void deleteObj(git_diff *ptr)
    {
        git_diff_free(ptr);
    }

    // Frees `git_status_list`.
    void deleteObj(git_status_list *ptr)
    {
        git_status_list_free(ptr);
    }

    // Frees `git_commit`.
    void deleteObj(git_commit *ptr)
    {
        git_commit_free(ptr);
    }

    // Frees `git_tree`.
    void deleteObj(git_tree *ptr)
    {
        git_tree_free(ptr);
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

}

static DiffEntryFile makeFileEntry(git_repository *repo,
                                   const git_diff_file &file,
                                   bool forceRead);
static std::string readObj(git_repository *repo, const git_oid &id);

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
        throw GitException("Could not discover repository");
    }
    BOOST_SCOPE_EXIT_ALL(&repoPath) { git_buf_free(&repoPath); };

    if (git_repository_open(&repo, repoPath.ptr) != 0) {
        throw GitException("Could not open repository");
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
        throw GitException("Failed to list status of files");
    }

    std::vector<DiffEntry> entries;
    for (size_t i = 0;
         const git_status_entry *entry = git_status_byindex(statusList, i);
         ++i) {
        git_diff_delta *delta = entry->head_to_index != nullptr
                              ? entry->head_to_index
                              : entry->index_to_workdir;

        // TODO: also handle added and removed files.
        if (delta->status == GIT_DELTA_MODIFIED) {
            entries.push_back({
                makeFileEntry(repo, delta->old_file, false),
                makeFileEntry(repo, delta->new_file, !staged)
            });
        }
    }
    return entries;
}

std::vector<DiffEntry>
Repository::listCommit(const std::string &ref)
{
    GitObjPtr<git_object> obj;
    if (git_revparse_single(&obj, repo, ref.c_str()) != 0) {
        throw std::invalid_argument("Failed to resolve ref: " + ref);
    }

    if (git_object_type(obj) != GIT_OBJ_COMMIT) {
        throw std::invalid_argument {
            std::string("Expected commit object, got ") +
            git_object_type2string(git_object_type(obj))
        };
    }

    auto *const commit = obj.as<const git_commit>();

    GitObjPtr<git_commit> parent;
    if (git_commit_parent(&parent, commit, 0) != 0) {
        throw std::invalid_argument("Failed to find parent of ref: " + ref);
    }

    GitObjPtr<git_tree> commitRoot;
    if (git_tree_lookup(&commitRoot, repo, git_commit_tree_id(commit)) != 0) {
        throw std::runtime_error("Failed to obtain tree root of a commit");
    }

    // TODO: handle commits with multiple parents in a special way?
    GitObjPtr<git_tree> parentRoot;
    if (git_tree_lookup(&parentRoot, repo, git_commit_tree_id(parent)) != 0) {
        throw std::runtime_error("Failed to obtain tree root of parent");
    }

    GitObjPtr<git_diff> diff;
    if (git_diff_tree_to_tree(&diff, repo, parentRoot, commitRoot,
                              nullptr) != 0) {
        throw std::runtime_error("Failed to diff trees");
    }

    std::vector<DiffEntry> entries;
    for (size_t i = 0;
         const git_diff_delta *delta = git_diff_get_delta(diff, i);
         ++i) {
        // TODO: also handle added and removed files.
        if (delta->status == GIT_DELTA_MODIFIED) {
            entries.push_back({ makeFileEntry(repo, delta->old_file, false),
                                makeFileEntry(repo, delta->new_file, false) });
        }
    }
    return entries;
}

// Builds a file entry out of diff data.
static DiffEntryFile
makeFileEntry(git_repository *repo, const git_diff_file &file, bool forceRead)
{
    if (git_oid_iszero(&file.id) || forceRead) {
        std::string path = git_repository_workdir(repo);
        path += '/';
        path += file.path;
        return DiffEntryFile(file.path, std::move(path));
    }

    return DiffEntryFile(file.path, file.path, readObj(repo, file.id));
}

// Fetches blob's content as a string.
static std::string
readObj(git_repository *repo, const git_oid &id)
{
    GitObjPtr<git_blob> blob;
    if (git_blob_lookup(&blob, repo, &id) != 0) {
        throw GitException("Failed to read a blob");
    }
    return std::string(static_cast<const char *>(git_blob_rawcontent(blob)),
                       static_cast<std::size_t>(git_blob_rawsize(blob)));
}
