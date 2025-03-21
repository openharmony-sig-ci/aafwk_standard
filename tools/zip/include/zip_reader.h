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
#ifndef FOUNDATION_AAFWK_STANDARD_TOOLS_ZIP_READER_H_
#define FOUNDATION_AAFWK_STANDARD_TOOLS_ZIP_READER_H_

#include <stddef.h>
#include <stdint.h>
#include <functional>
#include <memory>
#include <string>
#include <time.h>
#include <stdio.h>
#include "file_path.h"
#include "zip_utils.h"
#include "contrib/minizip/unzip.h"

namespace OHOS {
namespace AAFwk {
namespace LIBZIP {

// A delegate interface used to stream out an entry; see
// ZipReader::ExtractCurrentEntry.
class WriterDelegate {
public:
    virtual ~WriterDelegate()
    {}

    // Invoked once before any data is streamed out to pave the way (e.g., to open
    // the output file). Return false on failure to cancel extraction.
    virtual bool PrepareOutput() = 0;

    // Invoked to write the next chunk of data. Return false on failure to cancel
    // extraction.
    virtual bool WriteBytes(const char *data, int numBytes) = 0;

    // Sets the last-modified time of the data.
    virtual void SetTimeModified(const struct tm *modifiedTime) = 0;
};

// This class is used for reading zip files. A typical use case of this
// class is to scan entries in a zip file and extract them. The code will
// look like:
//
//   ZipReader reader;
//   reader.Open(zipFilePath);
//   while (reader.HasMore()) {
//     reader.OpenCurrentEntryInZip();
//     const base::FilePath& entry_path =
//        reader.CurrentEntryInfo()->file_path();
//     auto writer = CreateFilePathWriterDelegate(extract_dir, entry_path);
//     reader.ExtractCurrentEntry(writer, std::numeric_limits<uint64_t>::max());
//     reader.AdvanceToNextEntry();
//   }
//
// For simplicity, error checking is omitted in the example code above. The
// production code should check return values from all of these functions.
class ZipReader {
public:
    // A callback that is called when the operation is successful.
    // using SuccessCallback = base::OnceClosure;
    using SuccessCallback = std::function<void()>;
    // A callback that is called when the operation fails.
    using FailureCallback = std::function<void()>;
    // A callback that is called periodically during the operation with the number
    // of bytes that have been processed so far.
    // using ProgressCallback = base::RepeatingCallback<void(int64_t)>;
    using ProgressCallback = std::function<void(int64_t)>;

    // This class represents information of an entry (file or directory) in
    // a zip file.
    class EntryInfo {
    public:
        EntryInfo(const std::string &fileNameInZip, const unz_file_info &rawFileInfo);

        // Returns the file path. The path is usually relative like
        // "foo/bar.txt", but if it's absolute, is_unsafe() returns true.
        const FilePath &GetFilePath() const
        {
            return filePath_;
        }

        // Returns the size of the original file (i.e. after uncompressed).
        // Returns 0 if the entry is a directory.
        // Note: this value should not be trusted, because it is stored as metadata
        // in the zip archive and can be different from the real uncompressed size.
        int64_t GetOriginalSize() const
        {
            return originalSize_;
        }

        // Returns the last modified time. If the time stored in the zip file was
        // not valid, the unix epoch will be returned.
        struct tm GetLastModified() const
        {
            return lastModified_;
        }

        // Returns true if the entry is a directory.
        bool IsDirectory() const
        {
            return isDirectory_;
        }

        // Returns true if the entry is unsafe, like having ".." or invalid
        // UTF-8 characters in its file name, or the file path is absolute.
        bool IsUnsafe() const
        {
            return isUnsafe_;
        }

        // Returns true if the entry is encrypted.
        bool IsEncrypted() const
        {
            return isEncrypted_;
        }

    private:
        FilePath filePath_;
        int64_t originalSize_;
        struct tm lastModified_;
        bool isDirectory_;
        bool isUnsafe_;
        bool isEncrypted_;
        DISALLOW_COPY_AND_ASSIGN(EntryInfo);
    };

    ZipReader();
    ~ZipReader();

    // Opens the zip file specified by |zipFilePath|. Returns true on
    // success.
    bool Open(FilePath &zipFilePath);

    // Opens the zip file referred to by the platform file |zipFd|, without
    // taking ownership of |zipFd|. Returns true on success.
    bool OpenFromPlatformFile(PlatformFile zipFd);

    // Opens the zip data stored in |data|. This class uses a weak reference to
    // the given sring while extracting files, i.e. the caller should keep the
    // string until it finishes extracting files.
    bool OpenFromString(const std::string &data);

    // Closes the currently opened zip file. This function is called in the
    // destructor of the class, so you usually don't need to call this.
    void Close();

    // Returns true if there is at least one entry to read. This function is
    // used to scan entries with AdvanceToNextEntry(), like:
    //
    // while (reader.HasMore()) {
    //   // Do something with the current file here.
    //   reader.AdvanceToNextEntry();
    // }
    bool HasMore();

    // Advances the next entry. Returns true on success.
    bool AdvanceToNextEntry();

    // Opens the current entry in the zip file. On success, returns true and
    // updates the the current entry state (i.e. CurrentEntryInfo() is
    // updated). This function should be called before operations over the
    // current entry like ExtractCurrentEntryToFile().
    //
    // Note that there is no CloseCurrentEntryInZip(). The the current entry
    // state is reset automatically as needed.
    bool OpenCurrentEntryInZip();

    // Extracts |numBytesToExtract| bytes of the current entry to |delegate|,
    // starting from the beginning of the entry. Return value specifies whether
    // the entire file was extracted.
    bool ExtractCurrentEntry(WriterDelegate *delegate, uint64_t numBytesToExtract) const;

    // Extracts the current entry into memory. If the current entry is a
    // directory, the |output| parameter is set to the empty string. If the
    // current entry is a file, the |output| parameter is filled with its
    // contents. OpenCurrentEntryInZip() must be called beforehand. Note: the
    // |output| parameter can be filled with a big amount of data, avoid passing
    // it around by value, but by reference or pointer. Note: the value returned
    // by EntryInfo::original_size() cannot be trusted, so the real size of the
    // uncompressed contents can be different. |max_read_bytes| limits the ammount
    // of memory used to carry the entry. Returns true if the entire content is
    // read. If the entry is bigger than |max_read_bytes|, returns false and
    // |output| is filled with |max_read_bytes| of data. If an error occurs,
    // returns false, and |output| is set to the empty string.
    // bool ExtractCurrentEntryToString(uint64_t max_read_bytes, std::string *output) const;

    // Returns the current entry info. Returns NULL if the current entry is
    // not yet opened. OpenCurrentEntryInZip() must be called beforehand.
    EntryInfo *CurrentEntryInfo() const
    {
        return currentEntryInfo_.get();
    }

    // Returns the number of entries in the zip file.
    // Open() must be called beforehand.
    int num_entries() const
    {
        return numEntries_;
    }

private:
    // Common code used both in Open and OpenFromFd.
    bool OpenInternal();

    // Resets the internal state.
    void Reset();

    unzFile zipFile_;
    int numEntries_;
    bool reachedEnd_;
    std::unique_ptr<EntryInfo> currentEntryInfo_;

    DISALLOW_COPY_AND_ASSIGN(ZipReader);
};

// A writer delegate that writes a file at a given path.
class FilePathWriterDelegate : public WriterDelegate {
public:
    explicit FilePathWriterDelegate(const FilePath &outputFilePath);
    ~FilePathWriterDelegate() override;

    // WriterDelegate methods:

    // Creates the output file and any necessary intermediate directories.
    bool PrepareOutput() override;

    // Writes |numBytes| bytes of |data| to the file, returning false if not all
    // bytes could be written.
    bool WriteBytes(const char *data, int numBytes) override;

    // Sets the last-modified time of the data.
    void SetTimeModified(const struct tm *time) override;

private:
    FilePath outputFilePath_;
    FILE *file_;

    DISALLOW_COPY_AND_ASSIGN(FilePathWriterDelegate);
};

}  // namespace LIBZIP
}  // namespace AAFwk
}  // namespace OHOS
#endif  // FOUNDATION_AAFWK_STANDARD_TOOLS_ZIP_READER_H_
