#include "connect.h"
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <cstdlib>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

const int CURRENT_VERSION = 1;
const char* UPDATE_SERVER = "raw.githubusercontent.com";
const int UPDATE_PORT = 443;
const char* UPDATE_PATH = "/HubisAdmin2/hubisclass/main/version.json";

// HTTP GET isteği - WinHTTP ile HTTPS desteği (Windows)
std::string http_get(const char* host, int port, const char* path) {
    std::string response = "";
    
#ifdef _WIN32
    // WinHTTP ile HTTPS isteği
    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    
    // Session aç
    hSession = WinHttpOpen(L"HubisClass/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        printf("[WinHTTP] Session acma hatasi\n");
        return "";
    }
    
    // Bağlantı kur
    wchar_t w_host[256];
    MultiByteToWideChar(CP_UTF8, 0, host, -1, w_host, 256);
    hConnect = WinHttpConnect(hSession, w_host, port, 0);
    if (!hConnect) {
        printf("[WinHTTP] Baglanti hatasi\n");
        WinHttpCloseHandle(hSession);
        return "";
    }
    
    // Request oluştur
    wchar_t w_path[512];
    MultiByteToWideChar(CP_UTF8, 0, path, -1, w_path, 512);
    hRequest = WinHttpOpenRequest(hConnect, L"GET", w_path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        printf("[WinHTTP] Request acma hatasi\n");
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }
    
    // Timeout ayarları (240 saniye - 4 dakika)
    if (!WinHttpSetTimeouts(hRequest, 240000, 240000, 240000, 240000)) {
        printf("[WinHTTP] Timeout ayarlama hatasi\n");
    }
    
    // İsteği gönder
    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        printf("[WinHTTP] Request gonderme hatasi\n");
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }
    
    // Cevabı al
    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        printf("[WinHTTP] Response alma hatasi\n");
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }
    
    // Veriyi oku
    DWORD bytesAvailable = 0;
    DWORD totalRead = 0;
    response.reserve(10 * 1024 * 1024); // 10 MB reserve
    
    while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
        char* buffer = new char[bytesAvailable + 1];
        DWORD bytesRead = 0;
        
        if (WinHttpReadData(hRequest, buffer, bytesAvailable, &bytesRead)) {
            buffer[bytesRead] = '\0';
            response.append(buffer, bytesRead);
            totalRead += bytesRead;
            
            if (totalRead % (100 * 1024) < bytesRead) {
                printf("[WinHTTP] Indiriliyor: %lu KB\n", totalRead / 1024);
            }
        } else {
            printf("[WinHTTP] Okuma hatasi, hata kodu: %lu\n", GetLastError());
            delete[] buffer;
            break;
        }
        
        delete[] buffer;
    }
    
    printf("[WinHTTP] Toplam alinan: %lu KB\n", totalRead / 1024);
    
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    printf("[WinHTTP] Response boyutu: %zu bytes\n", response.size());
    
#else
    // Linux için eski HTTP (HTTPS desteklenmez)
    printf("[HTTP] Linux tarafinda HTTPS desteklenmiyor\n");
    return "";
#endif
    
    return response;
}

// HTTP POST isteği
std::string http_post(const char* host, int port, const char* path, const char* data) {
    std::string response = "";
    
#ifdef _WIN32
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        return "";
    }
#else
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return "";
    }
#endif

    // Timeout ayarı (30 saniye)
#ifdef _WIN32
    DWORD timeout = 30000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = 30;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif

    // Host çözümleme
    struct hostent* he = gethostbyname(host);
    if (!he) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return "";
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = *((struct in_addr*)he->h_addr);

    // Bağlan
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return "";
    }

    // HTTP POST isteği gönder
    char request[2048];
    snprintf(request, sizeof(request), 
             "POST %s HTTP/1.1\r\nHost: %s\r\nContent-Type: application/json\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n%s",
             path, host, strlen(data), data);
    
#ifdef _WIN32
    send(sock, request, strlen(request), 0);
#else
    send(sock, request, strlen(request), MSG_NOSIGNAL);
#endif

    // Cevap al
    char buffer[4096];
    int bytes_received;
    
    while ((bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        response += buffer;
    }

#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif

    // Header kısmını atla, sadece body al
    size_t body_start = response.find("\r\n\r\n");
    if (body_start != std::string::npos) {
        response = response.substr(body_start + 4);
    }

    return response;
}

UpdateInfo parse_update_json(const std::string& json) {
    UpdateInfo info;
    info.version = 0;
    info.filename = "";
    info.download_url = "";
    info.guncelleme = "";

    // Basit string arama (tam JSON parser değil)
    size_t version_pos = json.find("\"version\"");
    if (version_pos != std::string::npos) {
        size_t colon_pos = json.find(":", version_pos);
        if (colon_pos != std::string::npos) {
            size_t value_start = json.find_first_of("0123456789", colon_pos);
            if (value_start != std::string::npos) {
                info.version = std::atoi(json.substr(value_start).c_str());
            }
        }
    }

    size_t filename_pos = json.find("\"filename\"");
    if (filename_pos != std::string::npos) {
        size_t colon_pos = json.find(":", filename_pos);
        if (colon_pos != std::string::npos) {
            size_t quote_start = json.find("\"", colon_pos);
            if (quote_start != std::string::npos) {
                size_t quote_end = json.find("\"", quote_start + 1);
                if (quote_end != std::string::npos) {
                    info.filename = json.substr(quote_start + 1, quote_end - quote_start - 1);
                }
            }
        }
    }

    size_t url_pos = json.find("\"download_url\"");
    if (url_pos != std::string::npos) {
        size_t colon_pos = json.find(":", url_pos);
        if (colon_pos != std::string::npos) {
            size_t quote_start = json.find("\"", colon_pos);
            if (quote_start != std::string::npos) {
                size_t quote_end = json.find("\"", quote_start + 1);
                if (quote_end != std::string::npos) {
                    info.download_url = json.substr(quote_start + 1, quote_end - quote_start - 1);
                }
            }
        }
    }

    size_t guncelleme_pos = json.find("\"guncelleme\"");
    if (guncelleme_pos != std::string::npos) {
        size_t colon_pos = json.find(":", guncelleme_pos);
        if (colon_pos != std::string::npos) {
            size_t quote_start = json.find("\"", colon_pos);
            if (quote_start != std::string::npos) {
                size_t quote_end = json.find("\"", quote_start + 1);
                if (quote_end != std::string::npos) {
                    info.guncelleme = json.substr(quote_start + 1, quote_end - quote_start - 1);
                }
            }
        }
    }

    return info;
}

// Dosya indir
bool download_file(const std::string& url, const std::string& save_path) {
    printf("[Guncelleme] Dosya indiriliyor: %s\n", url.c_str());
    
    // URL'den host, port ve path çıkar
    size_t host_start = url.find("://");
    if (host_start == std::string::npos) {
        printf("[Guncelleme] HATA: URL formati hatali\n");
        return false;
    }
    
    host_start += 3;
    size_t path_start = url.find("/", host_start);
    if (path_start == std::string::npos) {
        printf("[Guncelleme] HATA: URL'de path bulunamadi\n");
        return false;
    }
    
    std::string host_port = url.substr(host_start, path_start - host_start);
    std::string path = url.substr(path_start);
    
    // Host ve port ayır
    std::string host = host_port;
    int port = UPDATE_PORT;
    size_t port_pos = host_port.find(":");
    if (port_pos != std::string::npos) {
        host = host_port.substr(0, port_pos);
        port = std::atoi(host_port.substr(port_pos + 1).c_str());
    }
    
    printf("[Guncelleme] Host: %s, Port: %d, Path: %s\n", host.c_str(), port, path.c_str());

    // Dosya indir
    std::string content = http_get(host.c_str(), port, path.c_str());
    if (content.empty()) {
        printf("[Guncelleme] HATA: Dosya icerigi bos veya indirme basarisiz\n");
        return false;
    }
    
    printf("[Guncelleme] Dosya boyutu: %zu bytes\n", content.size());
    
    // İlk 20 byte'ı logla (debug için)
    printf("[Guncelleme] Ilk 20 byte: ");
    for (size_t i = 0; i < 20 && i < content.size(); i++) {
        printf("%02X ", (unsigned char)content[i]);
    }
    printf("\n");

    // Dosyaya yaz
    std::ofstream file(save_path, std::ios::binary);
    if (!file.is_open()) {
        printf("[Guncelleme] HATA: Dosya acilamadi: %s\n", save_path.c_str());
        return false;
    }
    
    file.write(content.c_str(), content.size());
    file.close();
    
    printf("[Guncelleme] Dosya kaydedildi: %s\n", save_path.c_str());

    return true;
}

// Güncelleme kontrolü
void check_for_update() {
    printf("[Guncelleme] Kontrol baslatiliyor...\n");
    
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        printf("[Guncelleme] HATA: WSA baslatilamadi\n");
        return;
    }
#endif

    printf("[Guncelleme] Sunucuya baglaniyor: %s:%d%s\n", UPDATE_SERVER, UPDATE_PORT, UPDATE_PATH);
    std::string json_response = http_get(UPDATE_SERVER, UPDATE_PORT, UPDATE_PATH);
    if (json_response.empty()) {
        printf("[Guncelleme] HATA: Sunucuya baglanilamadi veya cevap yok\n");
        return;
    }

    printf("[Guncelleme] JSON alindi: %s\n", json_response.c_str());
    UpdateInfo info = parse_update_json(json_response);
    printf("[Guncelleme] Sunucu versiyonu: %d, Mevcut versiyon: %d, Guncelleme: %s\n", info.version, CURRENT_VERSION, info.guncelleme.c_str());
    
    if (info.version <= CURRENT_VERSION) {
        printf("[Guncelleme] Yeni versiyon yok\n");
        return;
    }

    if (info.guncelleme != "var") {
        printf("[Guncelleme] Guncelleme izni yok (guncelleme: %s)\n", info.guncelleme.c_str());
        return;
    }

    // Dosya zaten var mı kontrol et
    std::ifstream check_file(info.filename);
    if (check_file.good()) {
        check_file.close();
        printf("[Guncelleme] Dosya zaten var: %s, indirme atlandi\n", info.filename.c_str());
        return;
    }
    check_file.close();

    printf("[Guncelleme] Yeni versiyon bulundu! Indiriliyor: %s\n", info.filename.c_str());
    // Dosya indir
    std::string save_path = info.filename;
    if (!download_file(info.download_url, save_path)) {
        printf("[Guncelleme] HATA: Dosya indirme basarisiz\n");
        return;
    }

    printf("[Guncelleme] Dosya indirildi: %s\n", save_path.c_str());
    printf("[Guncelleme] Program kapaniyor...\n");

#ifdef _WIN32
    WSACleanup();
#endif
    
    exit(0);
}
