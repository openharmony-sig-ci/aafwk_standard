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
#ifndef FOUNDATION_AAFWK_STANDARD_TOOLS_ZIP_FILE_PATH_H
#define FOUNDATION_AAFWK_STANDARD_TOOLS_ZIP_FILE_PATH_H
#include <string>
#include <vector>
namespace OHOS {
namespace AAFwk {
namespace LIBZIP {

#define FILE_PATH_LITERAL(x) x
#define PRFilePath "s"

template <typename T, size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];
#define arraysize(array) (sizeof(ArraySizeHelper(array)))

class FilePath {
public:
    using CharType = std::string::value_type;

    FilePath();
    FilePath(const FilePath &that);
    explicit FilePath(const std::string &path);
    ~FilePath();
    FilePath &operator=(const FilePath &that);
    bool operator==(const FilePath &that) const;
    bool operator!=(const FilePath &that) const;
    bool operator<(const FilePath &that) const;

    static const CharType kSeparators[];
    static const size_t kSeparatorsLength;
    static const CharType kCurrentDirectory[];
    static const CharType kParentDirectory[];
    static const CharType kExtensionSeparator;

    static FilePath FromUTF8Unsafe(const std::string &utf8);
    static bool IsSeparator(CharType character);
    static bool CreateDirectory(const FilePath &fullPath);
    static bool DirectoryExists(const FilePath &path);
    static bool PathIsValid(const FilePath &path);

    // Returns a FilePath by appending a separator and the supplied path
    // component to this object's path.  Append takes care to avoid adding
    // excessive separators if this object's path already ends with a separator.
    // If this object's path is kCurrentDirectory, a new FilePath corresponding
    // only to |component| is returned.  |component| must be a relative path;
    // it is an error to pass an absolute path.
    FilePath Append(const std::string &component);
    FilePath Append(FilePath &component);
    // If IsParent(child) holds, appends to path (if non-NULL) the
    // relative path to child and returns true.  For example, if parent
    // holds "/Users/johndoe/Library/Application Support", child holds
    // "/Users/johndoe/Library/Application Support/Google/Chrome/Default",
    // and *path holds "/Users/johndoe/Library/Caches", then after
    // parent.AppendRelativePath(child, path) is called *path will hold
    // "/Users/johndoe/Library/Caches/Google/Chrome/Default".  Otherwise,
    // returns false.
    bool AppendRelativePath(const FilePath &child, FilePath *path);

    bool ReferencesParent();
    void GetComponents(std::vector<std::string> &components);
    FilePath DirName();
    FilePath BaseName();
    bool IsAbsolute();
    std::string Value();

private:
    std::string path_;

    void StripTrailingSeparatorsInternal();
    std::string::size_type FindDriveLetter(const std::string &path);
    bool AreAllSeparators(const std::string &input);
};
}  // namespace LIBZIP
}  // namespace AAFwk
}  // namespace OHOS

#endif  // FOUNDATION_AAFWK_STANDARD_TOOLS_ZIP_FILE_PATH_H