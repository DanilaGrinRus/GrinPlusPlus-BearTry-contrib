#include <catch.hpp>
#include <PMMR/Zip/ZipFile.h>
#include <minizip/zip.h>
#include <filesystem.h>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <random>
#include <string>
#include <vector>

TEST_CASE("ZipFile rejects a CRC-corrupted snapshot entry")
{
    const fs::path root = fs::temp_directory_path() / ("grinpp-zip-test-" + std::to_string(std::random_device{}()));
    REQUIRE(fs::create_directory(root));
    const fs::path archive = root / "snapshot.zip";
    const std::string payload = "snapshot-payload-for-crc-test";
    zipFile z = zipOpen(archive.u8string().c_str(), APPEND_STATUS_CREATE);
    REQUIRE(z != nullptr);
    zip_fileinfo info{};
    REQUIRE(zipOpenNewFileInZip(z, "entry", &info, nullptr, 0, nullptr, 0, nullptr, 0, 0) == ZIP_OK);
    REQUIRE(zipWriteInFileInZip(z, payload.data(), (unsigned int)payload.size()) == ZIP_OK);
    REQUIRE(zipCloseFileInZip(z) == ZIP_OK);
    REQUIRE(zipClose(z, nullptr) == ZIP_OK);
    REQUIRE_NOTHROW(ZipFile::Load(archive)->ExtractFile("entry", root / "valid"));

    std::ifstream in(archive, std::ios::binary);
    std::vector<char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    const auto pos = std::search(bytes.begin(), bytes.end(), payload.begin(), payload.end());
    REQUIRE(pos != bytes.end());
    *pos ^= 1;
    const fs::path damaged = root / "damaged.zip";
    std::ofstream out(damaged, std::ios::binary | std::ios::trunc);
    out.write(bytes.data(), (std::streamsize)bytes.size());
    out.close();
    REQUIRE(out.good());
    REQUIRE_THROWS(ZipFile::Load(damaged)->ExtractFile("entry", root / "damaged"));
    std::error_code ec;
    fs::remove_all(root, ec);
}
