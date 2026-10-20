#ifndef CONNECT_H
#define CONNECT_H

#include <string>

// Basit JSON parse - version, filename, download_url, guncelleme
struct UpdateInfo {
    int version;
    std::string filename;
    std::string download_url;
    std::string guncelleme;
};

// HTTP GET isteği
std::string http_get(const char* host, int port, const char* path);

// HTTP POST isteği
std::string http_post(const char* host, int port, const char* path, const char* data);

// JSON parse
UpdateInfo parse_update_json(const std::string& json);

// Dosya indir
bool download_file(const std::string& url, const std::string& save_path);

// Güncelleme kontrolü
void check_for_update();

#endif // CONNECT_H
