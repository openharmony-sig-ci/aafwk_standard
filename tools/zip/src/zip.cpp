/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "zip.h"
#include <list>
#include <string>
#include <vector>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "file_path.h"
#include "zip_internal.h"
#include "runnable.h"
#include "zip_reader.h"
#include "zip_writer.h"
#include "directory_ex.h"
#include "hilog_wrapper.h"

namespace OHOS {
namespace AAFwk {
namespace LIBZIP {
namespace {
using FilterCallback = std::function<bool(const FilePath &)>;
using DirectoryCreator = std::function<bool(FilePath &, FilePath &)>;
using WriterFactory = std::function<std::unique_ptr<WriterDelegate>(FilePath &, FilePath &)>;

const std::string SEPARATOR = "/";
const char HIDDEN_SEPARATOR = '.';

#define CALLING_CALL_BACK(callback, result) \
    if (callback != nullptr) {              \
        callback(result);                   \
    }

bool IsHiddenFile(const FilePath &filePath)
{
    FilePath localFilePath = filePath;
    std::string path = localFilePath.BaseName().Value();
    if (!localFilePath.Value().empty()) {
        return localFilePath.Value().c_str()[0] == HIDDEN_SEPARATOR;
    } else {
        return false;
    }
}

bool ExcludeNoFilesFilter(const FilePath &filePath)
{
    return true;
}

bool ExcludeHiddenFilesFilter(const FilePath &filePath)
{
    return !IsHiddenFile(filePath);
}

std::vector<FileAccessor::DirectoryContentEntry> ListDirectoryContent(const FilePath &filePath)
{
    FilePath curPath = filePath;

    std::vector<FileAccessor::DirectoryContentEntry> fileDirectoryVector;
    std::vector<std::string> filelist;
    GetDirFiles(curPath.Value(), filelist);
    for (auto it = filelist.begin(); !(*it).empty() && it != filelist.end(); it++) {
        fileDirectoryVector.push_back(
            FileAccessor::DirectoryContentEntry(FilePath(*it), FilePath::DirectoryExists(FilePath(*it))));
    }
    return fileDirectoryVector;
}

// Creates a directory at |extractDir|/|entryPath|, including any parents.
bool CreateDirectory(FilePath &extractDir, FilePath &entryPath)
{
    std::string path = extractDir.Value();
    if (EndsWith(path, SEPARATOR)) {
        return FilePath::CreateDirectory(FilePath(extractDir.Value() + entryPath.Value()));
    } else {
        return FilePath::CreateDirectory(FilePath(extractDir.Value() + "/" + entryPath.Value()));
    }
}

// Creates a WriterDelegate that can write a file at |extractDir|/|entryPath|.
std::unique_ptr<WriterDelegate> CreateFilePathWriterDelegate(FilePath &extractDir, FilePath entryPath)
{
    if (EndsWith(extractDir.Value(), SEPARATOR)) {
        return std::make_unique<FilePathWriterDelegate>(FilePath(extractDir.Value() + entryPath.Value()));
    } else {
        return std::make_unique<FilePathWriterDelegate>(FilePath(extractDir.Value() + "/" + entryPath.Value()));
    }
}
}  // namespace

ZipParams::ZipParams(const FilePath &srcDir, const FilePath &destFile) : srcDir_(srcDir), destFile_(destFile)
{}

// Does not take ownership of |fd|.
ZipParams::ZipParams(const FilePath &srcDir, int destFd) : srcDir_(srcDir), destFd_(destFd)
{}

bool Zip(const ZipParams &params, const OPTIONS &options, CALLBACK callback)
{
    const std::vector<FilePath> *filesToAdd = &params.GetFilesTozip();
    std::vector<FilePath> allRelativeFiles;
    FilePath paramPath = params.SrcDir();
    bool endIsSeparator = EndsWith(paramPath.Value(), SEPARATOR);
    if (filesToAdd->empty()) {
        filesToAdd = &allRelativeFiles;
        std::list<FileAccessor::DirectoryContentEntry> entries;
        if (endIsSeparator) {
            entries.push_back(FileAccessor::DirectoryContentEntry(params.SrcDir(), true /* is directory*/));
            FilterCallback filterCallback = params.GetFilterCallback();
            for (auto iter = entries.begin(); iter != entries.end(); ++iter) {
                const FilePath &constEntryPath = iter->path;
                if (iter != entries.begin() && ((!params.GetIncludeHiddenFiles() && IsHiddenFile(constEntryPath)) ||
                                                   (filterCallback && !filterCallback(constEntryPath)))) {
                    continue;
                }
                if (iter != entries.begin()) {
                    FilePath relativePath;
                    FilePath entryPath = constEntryPath;
                    FilePath paramsSrcPath = params.SrcDir();
                    bool success = paramsSrcPath.AppendRelativePath(entryPath, &relativePath);
                    if (success) {
                        allRelativeFiles.push_back(relativePath);
                    }
                }
                if (iter->isDirectory) {
                    std::vector<FileAccessor::DirectoryContentEntry> subEntries = ListDirectoryContent(constEntryPath);
                    entries.insert(entries.end(), subEntries.begin(), subEntries.end());
                }
            }
        } else {
            allRelativeFiles.push_back(paramPath.BaseName());
        }
    }
    std::unique_ptr<ZipWriter> zipWriter;
    FilePath rootPath = (endIsSeparator == false) ? FilePath(paramPath.DirName().Value() + SEPARATOR) : params.SrcDir();
    if (params.DestFd() != kInvalidPlatformFile) {
        zipWriter = ZipWriter::CreateWithFd(params.DestFd(), rootPath);
        if (!zipWriter) {
            return false;
        }
    }
    if (!zipWriter) {
        zipWriter = ZipWriter::Create(params.DestFile(), rootPath);
        if (!zipWriter) {
            return false;
        }
    }
    return zipWriter->WriteEntries(*filesToAdd, options, callback);
}

bool UnzipWithFilterAndWriters(const PlatformFile &srcFile, FilePath &destDir, WriterFactory writerFactory,
    DirectoryCreator directoryCreator, CALLBACK callback, FilterCallback filterCB, bool logSkippedFiles)
{
    ZipReader reader;
    if (!reader.OpenFromPlatformFile(srcFile)) {
        CALLING_CALL_BACK(callback, ERROR_CODE_ERRNO)
        HILOG_INFO("%{public}s called, Failed to open srcFile.", __func__);
        return false;
    }
    while (reader.HasMore()) {
        if (!reader.OpenCurrentEntryInZip()) {
            CALLING_CALL_BACK(callback, ERROR_CODE_ERRNO)
            HILOG_INFO("%{public}s called, Failed to open the current file in zip.", __func__);
            return false;
        }
        const FilePath &constEntryPath = reader.CurrentEntryInfo()->GetFilePath();
        if (reader.CurrentEntryInfo()->IsUnsafe()) {
            CALLING_CALL_BACK(callback, ERROR_CODE_ERRNO)
            HILOG_INFO("%{public}s called, Found an unsafe file in zip.", __func__);
            return false;
        }
        FilePath entryPath = constEntryPath;
        // callback
        if (filterCB(entryPath)) {
            if (reader.CurrentEntryInfo()->IsDirectory()) {
                if (!directoryCreator(destDir, entryPath)) {
                    CALLING_CALL_BACK(callback, ERROR_CODE_ERRNO)
                    return false;
                }

            } else {
                std::unique_ptr<WriterDelegate> writer = writerFactory(destDir, entryPath);
                if (!reader.ExtractCurrentEntry(writer.get(), std::numeric_limits<uint64_t>::max())) {
                    CALLING_CALL_BACK(callback, ERROR_CODE_ERRNO)
                    HILOG_INFO("%{public}s called, Failed to extract.", __func__);
                    return false;
                }
            }
        } else if (logSkippedFiles) {
            HILOG_INFO("%{public}s called, Skipped file.", __func__);
        }

        if (!reader.AdvanceToNextEntry()) {
            CALLING_CALL_BACK(callback, ERROR_CODE_ERRNO)
            HILOG_INFO("%{public}s called, Failed to advance to the next file.", __func__);
            return false;
        }
    }
    CALLING_CALL_BACK(callback, ERROR_CODE_OK)
    return true;
}

bool UnzipWithFilterCallback(const FilePath &srcFile, const FilePath &destDir, const OPTIONS &options,
    CALLBACK callback, FilterCallback filterCB, bool logSkippedFiles)
{
    FilePath src = srcFile;
    FilePath dest = destDir;

    if (!FilePath::PathIsValid(srcFile)) {
        CALLING_CALL_BACK(callback, ERROR_CODE_DATA_ERROR)
        HILOG_INFO("%{public}s called, Failed to open.", __func__);
        return false;
    }

    PlatformFile zipFd = open(src.Value().c_str(), S_IREAD);

    bool ret = UnzipWithFilterAndWriters(zipFd,
        dest,
        std::bind(&CreateFilePathWriterDelegate, std::placeholders::_1, std::placeholders::_2),
        std::bind(&CreateDirectory, std::placeholders::_1, std::placeholders::_2),
        callback,
        filterCB,
        logSkippedFiles);

    close(zipFd);

    return ret;
}
bool Unzip(const FilePath &srcFile, const FilePath &destDir, const OPTIONS &options, CALLBACK callback)
{
    std::shared_ptr<Runnable> innerTask = std::make_shared<Runnable>([srcFile, destDir, options, callback]() {
        UnzipWithFilterCallback(srcFile, destDir, options, callback, ExcludeNoFilesFilter, true);
    });

    PostTask(innerTask);
    return true;
}

bool ZipWithFilterCallback(const FilePath &srcDir, const FilePath &destFile, const OPTIONS &options, CALLBACK callback,
    FilterCallback filterCB)
{
    FilePath srcPath = srcDir;
    if (!EndsWith(srcPath.Value(), SEPARATOR)) {
        if (!FilePath::DirectoryExists(srcPath.DirName())) {
            return false;
        }
    } else if (!FilePath::DirectoryExists(srcDir)) {
        return false;
    }
    ZipParams params(srcDir, destFile);
    params.SetFilterCallback(filterCB);
    return Zip(params, options, callback);
}

bool Zip(const FilePath &srcDir, const FilePath &destFile, const OPTIONS &options, CALLBACK callback,
    bool includeHiddenFiles)
{
    FilePath srctemp = srcDir;
    FilePath desttemp = destFile;
    HILOG_INFO("%{public}s called,  srcDir=%{public}s, destFile=%{public}s",
        __func__,
        srctemp.Value().c_str(),
        desttemp.Value().c_str());

    FilePath srcdir = srcDir;

    std::shared_ptr<Runnable> innerTask =
        std::make_shared<Runnable>([srcDir, destFile, includeHiddenFiles, callback]() {
            OPTIONS options;
            if (includeHiddenFiles) {
                ZipWithFilterCallback(srcDir, destFile, options, callback, ExcludeNoFilesFilter);
            } else {
                ZipWithFilterCallback(srcDir, destFile, options, callback, ExcludeHiddenFilesFilter);
            }
        });
    PostTask(innerTask);

    return true;
}
}  // namespace LIBZIP
}  // namespace AAFwk
}  // namespace OHOS
