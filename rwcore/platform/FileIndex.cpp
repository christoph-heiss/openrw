#include "platform/FileIndex.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>

#include "platform/FileHandle.hpp"
#include "rw/debug.hpp"

namespace {

std::string normalizeFilePath(const std::string& filePath) {
    std::string result(filePath.length(), '\0');

    std::transform(
        filePath.cbegin(), filePath.cend(), result.begin(),
        [](char c) -> char {
            if (c == '\\') {
                return '/';
            }
            return std::tolower(c);
        }
    );

    return result;
}

} // namespace

void FileIndex::indexTree(const std::filesystem::path &path) {
    // Remove the trailing "/" or "/." from base_path.
    std::filesystem::path basePath = (path / ".").lexically_normal().parent_path();

    for (const auto& entry : std::filesystem::recursive_directory_iterator(basePath)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        std::filesystem::path relPath = entry.path().lexically_relative(basePath);
        std::string relNormPath = normalizeFilePath(relPath.string());
        indexedData_[relNormPath] = {IndexedDataType::FILE, entry.path(), ""};

        std::string filename = normalizeFilePath(entry.path().filename().string());
        indexedData_[filename] = {IndexedDataType::FILE, entry.path(), ""};
    }
}

const FileIndex::IndexedData* FileIndex::getIndexedDataAt(const std::filesystem::path &filePath) const {
    std::string normPath = normalizeFilePath(filePath.string());
    return &indexedData_.at(normPath);
}

const std::filesystem::path& FileIndex::findFilePath(const std::filesystem::path &filePath) const {
    return getIndexedDataAt(filePath)->path;
}

FileContentsInfo FileIndex::openFileRaw(const std::filesystem::path &filePath) const {
    const IndexedData* indexData = getIndexedDataAt(filePath);
    std::ifstream dfile(indexData->path, std::ios::binary);
    if (!dfile.is_open()) {
        throw std::runtime_error("Unable to open file: " + filePath.string());
    }

#ifdef RW_DEBUG
    if (indexData->type != IndexedDataType::FILE) {
        RW_MESSAGE("Reading raw data from archive \"" << filePath << "\"");
    }
#endif

    dfile.seekg(0, std::ios::end);
    auto length = dfile.tellg();
    dfile.seekg(0);
    auto data = std::make_unique<char[]>(length);
    dfile.read(data.get(), length);

    return {std::move(data), static_cast<size_t>(length)};
}

void FileIndex::indexArchive(const std::filesystem::path &archive) {
    const std::string& path = findFilePath(archive).string();

    LoaderIMG& img = loaders_[path];
    if (!img.load(path)) {
        throw std::runtime_error("Failed to load IMG archive: " + path);
    }

    for (size_t i = 0; i < img.getAssetCount(); ++i) {
        auto &asset = img.getAssetInfoByIndex(i);

        if (asset.size == 0) continue;

        std::string assetName = normalizeFilePath(asset.name);
        indexedData_[assetName] = {IndexedDataType::ARCHIVE, path, asset.name};
    }
}

FileContentsInfo FileIndex::openFile(const std::filesystem::path &filePath) {
    std::string normFilePath = normalizeFilePath(filePath.string());
    auto indexedDataPos = indexedData_.find(normFilePath);

    if (indexedDataPos == indexedData_.end()) {
        return {nullptr, 0};
    }

    const IndexedData& indexedData = indexedDataPos->second;

    std::unique_ptr<char[]> data = nullptr;
    size_t length = 0;

    if (indexedData.type == IndexedDataType::ARCHIVE) {
        auto loaderPos = loaders_.find(indexedData.path.string());
        if (loaderPos == loaders_.end()) {
            throw std::runtime_error("IMG archive not indexed: " + indexedData.path.string());
        }

        LoaderIMG& loader = loaderPos->second;
        LoaderIMGFile file;
        std::string filename = std::filesystem::path(indexedData.assetData).filename().string();
        if (loader.findAssetInfo(filename, file)) {
            length = file.size * 2048;
            data = loader.loadToMemory(filename);
        }
    } else {
        std::ifstream dfile(indexedData.path.string(), std::ios::binary);
        if (!dfile.is_open()) {
            throw std::runtime_error("Unable to open file: " + indexedData.path.string());
        }

        dfile.seekg(0, std::ios::end);
        length = dfile.tellg();
        dfile.seekg(0);
        data = std::make_unique<char[]>(length);
        dfile.read(data.get(), length);
    }

    return {std::move(data), length};
}
