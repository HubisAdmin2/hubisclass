#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "connect.h"
#include <GLFW/glfw3.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <windows.h>
#endif

#include <stdio.h>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <algorithm>
#include <random>
#include <string>
#include <fstream>
#include "arial_font.h"
#include "rose_emoji.h"

static bool show_about = false;
static bool show_reset_confirm = false;
static bool use_rose_emoji = false;
static GLuint rose_emoji_texture = 0;
static int class_size = 30;
static int selected_student = 0;
static bool student_selected = false;
static std::vector<int> available_students;
static bool window_was_dragged = false;
static bool is_dragging = false;
static double drag_start_x = 0.0;
static double drag_start_y = 0.0;
static int window_start_x = 0;
static int window_start_y = 0;
static bool is_resizing = false;
static bool is_resizing_right = false;
static bool is_resizing_bottom = false;
static bool is_resizing_left = false;
static bool is_resizing_top = false;
static double resize_start_x = 0.0;
static double resize_start_y = 0.0;
static int resize_start_width = 0;
static int resize_start_height = 0;

// Fonksiyon bildirimleri
void check_for_update();

// Font değişkenleri (global)
static float font_large_size = 18.0f;
static ImFont* font_normal = nullptr;
static ImFont* font_large = nullptr;

// Animasyon değişkenleri
static bool animation_running = false;
static double animation_start_time = 0.0;
static int animation_display_student = 0;
static int final_selected_student = 0;
static const double animation_duration = 0.2; // 0.2 saniye
static double animation_end_time = 0.0; // Animasyon bitiş zamanı

void ResetStudentPool(int size) {
    available_students.clear();
    available_students.reserve(size); // Pre-allocate memory
    for (int i = 1; i <= size; ++i) {
        available_students.push_back(i);
    }
    
    // Sattolo's Cycle algoritması - hiçbir eleman yerinde kalmaz
    static std::mt19937 g(std::random_device{}());
    for (int i = size - 1; i > 0; --i) {
        std::uniform_int_distribution<int> dist(0, i - 1);
        int j = dist(g);
        std::swap(available_students[i], available_students[j]);
    }
    
    // 31 kişilikse 19'un şansını artır
    if (size == 31) {
        // 19'un pozisyonunu bul
        auto it = std::find(available_students.begin(), available_students.end(), 19);
        if (it != available_students.end()) {
            int pos_19 = it - available_students.begin();
            // Eğer son 15 içindeyse, %0.001 ihtimalle ilk 10'a taşı
            if (pos_19 >= size - 15) {
                std::uniform_real_distribution<double> chance(0.0, 1.0);
                if (chance(g) < 0.01) { // %1 şans
                    int new_pos = std::uniform_int_distribution<int>(0, 9)(g);
                    std::swap(available_students[pos_19], available_students[new_pos]);
                }
            }
        }
    }
}

void SaveData() {
    // Kayıt sistemi devre dışı
}

void LoadData() {
    // Yükleme sistemi devre dışı
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static void glfw_framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void OpenURL(const char* url) {
    char command[512];
    snprintf(command, sizeof(command), "start %s", url);
    system(command);
}

int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

#ifdef _WIN32
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif

    // Güncelleme kontrolü (sessiz)
    check_for_update();

    const char* glsl_version = "#version 120";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); // Üst barsız

    // Ekran çözünürlüğünü al
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    int screen_width = mode->width;
    int screen_height = mode->height;

    // Pencere boyutunu sabit 300x400 piksel olarak ayarla
    int window_width = 300;
    int window_height = 400;

    GLFWwindow* window = glfwCreateWindow(window_width, window_height, "HubisClass", NULL, NULL);
    if (window == NULL)
        return 1;

    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);

    glfwSetWindowAttrib(window, GLFW_FLOATING, GLFW_TRUE);
    glfwSetWindowAttrib(window, GLFW_RESIZABLE, GLFW_FALSE); // Manuel resize handle (Wine uyumlu)

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0); // VSync kapalı - daha akıcı taşıma için

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = NULL; // imgui.ini oluşturma
    
    // Load Arial font from embedded data (portable for Wine/Linux)
    ImFontConfig font_config;
    font_config.OversampleH = 3; // Horizontal oversampling for crisp text
    font_config.OversampleV = 3; // Vertical oversampling for crisp text
    font_config.PixelSnapH = false; // Disable pixel snapping for smooth rendering
    
    // Dinamik font boyutları - pencere boyutuna göre (%5.5)
    float font_normal_size = window_width * 0.055f; // Pencere genişliğinin %5.5'i
    font_large_size = window_width * 0.055f; // Pencere genişliğinin %5.5'i (global)
    
    // Minimum font boyutları
    if (font_normal_size < 14.0f) font_normal_size = 14.0f;
    if (font_large_size < 20.0f) font_large_size = 20.0f;
    
    // Ana font (Arial)
    static const ImWchar arial_ranges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0100, 0x017F, // Latin Extended-A
        0x0180, 0x024F, // Latin Extended-B
        0,
    };
    font_normal = io.Fonts->AddFontFromMemoryTTF((void*)arial_ttf, arial_ttf_size, font_normal_size, &font_config, arial_ranges);
    
    font_large = font_normal;
    
    // Increase font atlas size for emoji support
    io.Fonts->TexDesiredWidth = 2048;
    io.Fonts->Build();

    // LoadData(); // Data yükleme devre dışı
    
    // Başlangıçta öğrenci havuzunu oluştur
    ResetStudentPool(class_size);

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Dinamik padding ve spacing - pencere boyutuna göre (artırıldı)
    style.WindowPadding = ImVec2(window_width * 0.03f, window_height * 0.03f);
    style.FramePadding = ImVec2(window_width * 0.03f, window_height * 0.02f);
    style.ItemSpacing = ImVec2(window_width * 0.03f, window_height * 0.02f);
    style.ItemInnerSpacing = ImVec2(window_width * 0.015f, window_height * 0.015f);
    
    style.WindowRounding = 14.0f;
    style.FrameRounding = 10.0f;
    style.PopupRounding = 12.0f;
    style.WindowBorderSize = 0.0f;
    
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.07f, 0.12f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.13f, 0.48f, 0.95f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.55f, 1.00f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.10f, 0.40f, 0.82f, 1.00f);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Rose emoji texture oluştur (OpenGL context başlatıldıktan sonra)
    glGenTextures(1, &rose_emoji_texture);
    glBindTexture(GL_TEXTURE_2D, rose_emoji_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rose_emoji_width, rose_emoji_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rose_emoji_data);
    glBindTexture(GL_TEXTURE_2D, 0);

    ImVec4 clear_color = ImVec4(0.02f, 0.03f, 0.06f, 1.00f);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Pencere boyutunu her frame'de güncelle
        glfwGetWindowSize(window, &window_width, &window_height);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2((float)window_width, (float)window_height), ImGuiCond_Always);
        ImGui::SetNextWindowSizeConstraints(ImVec2(300, 400), ImVec2(FLT_MAX, FLT_MAX));

        ImGui::Begin("HubisClass", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

        // Pencere Taşıma - Windows API ile native taşıma (resize sırasında devre dışı)
#ifdef _WIN32
        if (!is_dragging && !is_resizing && ImGui::IsMouseClicked(0)) {
            ImVec2 mouse_pos = ImGui::GetMousePos();
            if (mouse_pos.y < 50 && !show_about && !show_reset_confirm) {
                HWND hwnd = glfwGetWin32Window(window);
                if (hwnd) {
                    ReleaseCapture();
                    SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
                    is_dragging = true;
                }
            }
        }
        
        if (is_dragging && !ImGui::IsMouseDown(0)) {
            is_dragging = false;
        }
#else
        // Linux/Wine için manuel taşıma (resize sırasında devre dışı)
        if (!is_dragging && !is_resizing && ImGui::IsMouseClicked(0)) {
            ImVec2 mouse_pos = ImGui::GetMousePos();
            if (mouse_pos.y < 50 && !show_about && !show_reset_confirm) {
                is_dragging = true;
                glfwGetCursorPos(window, &drag_start_x, &drag_start_y);
                glfwGetWindowPos(window, &window_start_x, &window_start_y);
            }
        }
        
        if (is_dragging && ImGui::IsMouseDown(0)) {
            double current_x, current_y;
            glfwGetCursorPos(window, &current_x, &current_y);
            glfwSetWindowPos(window, window_start_x + (int)(current_x - drag_start_x), window_start_y + (int)(current_y - drag_start_y));
        }
        
        if (is_dragging && !ImGui::IsMouseDown(0)) {
            is_dragging = false;
        }
#endif

        // Başlık (Büyük ve Ferah)
        ImGui::SetCursorPos(ImVec2(24, 16));
        ImGui::PushFont(font_large);
        ImGui::TextColored(ImVec4(0.35f, 0.80f, 1.00f, 1.00f), "HubisClass");
        ImGui::PopFont();

        // Küçült (-) ve Kapat (X) Butonları
        ImGui::SameLine(window_width - 72);
        ImGui::SetCursorPosY(12);
        if (ImGui::Button("-", ImVec2(32, 32))) {
            glfwIconifyWindow(window);
        }
        ImGui::SameLine(window_width - 34);
        ImGui::SetCursorPosY(12);
        if (ImGui::Button("X", ImVec2(32, 32))) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            exit(0); // Direkt kapanma
        }

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Spacing();

        // 1. Sinif Mevcudu Girisi - Flex yapısı
        ImGui::SetCursorPosX(20);
        ImGui::PushFont(font_normal);
        ImGui::Text("Sinif Mevcudu:");
        ImGui::SameLine();
        ImGui::SetCursorPosX(140);
        ImGui::SetNextItemWidth(100);
        if (ImGui::InputInt("##class_size", &class_size, 1, 10)) {
            if (class_size < 1) class_size = 1;
            if (class_size > 1000) class_size = 1000;
            ResetStudentPool(class_size);
            student_selected = false;
        }
        ImGui::PopFont();

        ImGui::Spacing();

        // Kalan Ogrenci Bilgisi - Flex yapısı
        ImGui::SetCursorPosX(20);
        char kalanStr[64];
        snprintf(kalanStr, sizeof(kalanStr), "Kalan Ogrenci: %d / %d", (int)available_students.size(), class_size);
        ImGui::PushFont(font_normal);
        ImGui::TextDisabled("%s", kalanStr);
        ImGui::PopFont();

        ImGui::Spacing();

        // 2. Ogrenci Sec Butonu - Flex yapısı
        ImGui::SetCursorPosX((window_width - window_width * 0.5f) / 2);
        ImGui::PushFont(font_large);
        
        // Cooldown kontrolü - animasyon bittikten sonra 2 saniye bekle
        bool button_enabled = true;
        if (animation_end_time > 0.0) {
            double current_time = glfwGetTime();
            if (current_time - animation_end_time < 0.3) {
                button_enabled = false;
            } else {
                animation_end_time = 0.0; // Cooldown bitti
            }
        }
        
        // Animasyon sırasında da disable
        if (animation_running) {
            button_enabled = false;
        }
        
        if (!button_enabled) {
            ImGui::BeginDisabled();
        }
        
        if (ImGui::Button("Ogrenci Sec", ImVec2(window_width * 0.5f, window_height * 0.08f)))
        {
            if (available_students.empty()) {
                show_reset_confirm = true;
            }
            if (!available_students.empty()) {
                // Animasyonlu seçim - 0.2 saniye slot makinesi efekti
                final_selected_student = available_students.back();
                available_students.pop_back();
                animation_running = true;
                animation_start_time = glfwGetTime();
                student_selected = false; // Animasyon bitene kadar false
            }
        }
        
        if (!button_enabled) {
            ImGui::EndDisabled();
        }
        
        ImGui::PopFont();

        ImGui::Spacing();
        ImGui::Spacing();

        // 3. Secilen Ogrenci Alani
        ImGui::SetCursorPosX((window_width - ImGui::CalcTextSize("SECILEN OGRENCI").x) / 2);
        ImGui::PushFont(font_normal);
        ImGui::TextDisabled("SECILEN OGRENCI");
        ImGui::PopFont();
        
        ImGui::Spacing();

        float circle_size = window_width * 0.25f;
        ImGui::SetCursorPosX((window_width - circle_size) / 2);
        
        // Animasyon mantığı - slot makinesi efekti
        if (animation_running) {
            double current_time = glfwGetTime();
            double elapsed = current_time - animation_start_time;
            
            if (elapsed >= animation_duration) {
                // Animasyon bitti - final seçim
                animation_running = false;
                animation_end_time = glfwGetTime(); // Cooldown başlat
                selected_student = final_selected_student;
                student_selected = true;
            } else {
                // Animasyon devam ediyor - hızlı rastgele sayı göster
                static std::mt19937 rng(std::random_device{}());
                std::uniform_int_distribution<int> dist(1, class_size);
                animation_display_student = dist(rng);
                selected_student = animation_display_student;
                student_selected = true; // Geçici olarak true
            }
        }
        
        if (student_selected)
        {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.15f, 0.28f, 1.00f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, circle_size * 0.8f); // Yuvarlatılmış köşeler
            ImGui::BeginChild("CircleBadge", ImVec2(circle_size, circle_size), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            
            char numStr[32];
            snprintf(numStr, sizeof(numStr), "%d", selected_student);
            
            // Sayıyı kareye tam sığacak şekilde ölçekle
            float original_scale = ImGui::GetIO().FontGlobalScale;
            ImGui::GetIO().FontGlobalScale = 1.0f;
            ImGui::PushFont(font_large);
            ImVec2 baseTextSize = ImGui::CalcTextSize(numStr);
            ImGui::PopFont();
            
            float padding = circle_size * 0.08f;
            float max_w = circle_size - padding * 2.0f;
            float max_h = circle_size - padding * 2.0f;
            float fit_scale = (std::min)(max_w / baseTextSize.x, max_h / baseTextSize.y);
            if (fit_scale < 0.1f) fit_scale = 0.1f;
            ImGui::GetIO().FontGlobalScale = fit_scale;
            
            ImGui::PushFont(font_large);
            ImVec2 textSize = ImGui::CalcTextSize(numStr);
            
            // Gül emojisi (use_rose_emoji açıksa, sınıf 31 ve numara 6)
            if (use_rose_emoji && class_size == 31 && selected_student == 6) {
                float emoji_size = circle_size * 0.3f;
                float total_width = textSize.x + emoji_size + 5;
                ImGui::SetCursorPos(ImVec2((circle_size - total_width) / 2, (circle_size - textSize.y) / 2));
                ImGui::TextColored(ImVec4(0.35f, 0.80f, 1.00f, 1.00f), "%s", numStr);
                ImGui::SameLine(0, 5);
                ImGui::Image((ImTextureID)(intptr_t)rose_emoji_texture, ImVec2(emoji_size, emoji_size));
            } else {
                ImGui::SetCursorPos(ImVec2((circle_size - textSize.x) / 2, (circle_size - textSize.y) / 2));
                ImGui::TextColored(ImVec4(0.35f, 0.80f, 1.00f, 1.00f), "%s", numStr);
            }
            ImGui::PopFont();
            
            ImGui::GetIO().FontGlobalScale = original_scale;
            
            ImGui::EndChild();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.10f, 0.16f, 1.00f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, circle_size * 0.8f); // Yuvarlatılmış köşeler
            ImGui::BeginChild("CircleBadgeEmpty", ImVec2(circle_size, circle_size), false, ImGuiWindowFlags_NoScrollbar);
            
            ImGui::PushFont(font_large);
            ImVec2 textSize = ImGui::CalcTextSize("-");
            ImGui::SetCursorPos(ImVec2((circle_size - textSize.x) / 2, (circle_size - textSize.y) / 2));
            ImGui::TextDisabled("-");
            ImGui::PopFont();
            
            ImGui::EndChild();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        }

        // Tüm Köşeler Resize - Mouse pozisyonu kontrolü
        ImVec2 mouse_pos = ImGui::GetMousePos();
        float resize_zone = 20.0f;
        
        // Resize başlatma - hangi köşe
        if (!is_resizing && ImGui::IsMouseClicked(0)) {
            if (mouse_pos.x > window_width - resize_zone && mouse_pos.y > window_height - resize_zone) {
                is_resizing = true;
                is_resizing_right = true;
                is_resizing_bottom = true;
                glfwGetCursorPos(window, &resize_start_x, &resize_start_y);
                glfwGetWindowSize(window, &resize_start_width, &resize_start_height);
                glfwGetWindowPos(window, &window_start_x, &window_start_y);
            }
            else if (mouse_pos.x > window_width - resize_zone && mouse_pos.y < resize_zone) {
                is_resizing = true;
                is_resizing_right = true;
                is_resizing_top = true;
                glfwGetCursorPos(window, &resize_start_x, &resize_start_y);
                glfwGetWindowSize(window, &resize_start_width, &resize_start_height);
                glfwGetWindowPos(window, &window_start_x, &window_start_y);
            }
            else if (mouse_pos.x < resize_zone && mouse_pos.y > window_height - resize_zone) {
                is_resizing = true;
                is_resizing_left = true;
                is_resizing_bottom = true;
                glfwGetCursorPos(window, &resize_start_x, &resize_start_y);
                glfwGetWindowSize(window, &resize_start_width, &resize_start_height);
                glfwGetWindowPos(window, &window_start_x, &window_start_y);
            }
            else if (mouse_pos.x < resize_zone && mouse_pos.y < resize_zone) {
                is_resizing = true;
                is_resizing_left = true;
                is_resizing_top = true;
                glfwGetCursorPos(window, &resize_start_x, &resize_start_y);
                glfwGetWindowSize(window, &resize_start_width, &resize_start_height);
                glfwGetWindowPos(window, &window_start_x, &window_start_y);
            }
        }
        
        // Resize mantığı
        if (is_resizing && ImGui::IsMouseDown(0)) {
            double current_x, current_y;
            glfwGetCursorPos(window, &current_x, &current_y);
            int new_width = resize_start_width;
            int new_height = resize_start_height;
            int new_x = window_start_x;
            int new_y = window_start_y;
            
            if (is_resizing_right) {
                new_width = resize_start_width + (int)(current_x - resize_start_x);
            }
            if (is_resizing_bottom) {
                new_height = resize_start_height + (int)(current_y - resize_start_y);
            }
            if (is_resizing_left) {
                int width_diff = (int)(resize_start_x - current_x);
                new_width = resize_start_width + width_diff;
                new_x = window_start_x - width_diff;
            }
            if (is_resizing_top) {
                int height_diff = (int)(resize_start_y - current_y);
                new_height = resize_start_height + height_diff;
                new_y = window_start_y - height_diff;
            }
            
            if (new_width < 300) new_width = 300;
            if (new_height < 400) new_height = 400;
            
            glfwSetWindowSize(window, new_width, new_height);
            if (is_resizing_left || is_resizing_top) {
                glfwSetWindowPos(window, new_x, new_y);
            }
        }
        
        if (is_resizing && !ImGui::IsMouseDown(0)) {
            is_resizing = false;
            is_resizing_right = false;
            is_resizing_bottom = false;
            is_resizing_left = false;
            is_resizing_top = false;
        }

        // Alt Telif Butonu - Flex yapısı
        ImGui::SetCursorPosY(window_height - 50);
        ImGui::SetCursorPosX(20);
        ImGui::PushFont(font_normal);
        if (ImGui::Button("Tum haklari HUBIS TEAM'e aittir.", ImVec2(window_width - 40, window_height * 0.06f)))
        {
            show_about = true;
        }
        ImGui::PopFont();

        // Hakkimizda Modali
        if (show_about)
        {
            ImGui::SetNextWindowPos(ImVec2(20, 50), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(240, 300), ImGuiCond_Always);

            if (ImGui::Begin("Hakkimizda", &show_about, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
            {
                ImGui::Spacing();
                ImGui::PushFont(font_normal);
                ImGui::TextWrapped("Hubisgame Bilisim ve Dijital Hizmetler, yenilikci dijital projeler ve yazilim cozumleri gelistiren profesyonel bir olusumdur.");
                ImGui::PopFont();
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                ImGui::PushFont(font_normal);
                ImGui::TextWrapped("Saimbey Ortaokulu ogretmeni Adem Ilengiz'e ithaf edilmistir.");
                ImGui::PopFont();
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::PushFont(font_normal);
                ImGui::Text("Diger Projelerimiz:");
                ImGui::PopFont();
                ImGui::Spacing();
                ImGui::PushFont(font_large);
                if (ImGui::Button("hubisgame.site", ImVec2(220, 35)))
                {
                    OpenURL("https://hubisgame.site");
                }
                ImGui::PopFont();

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::SetCursorPosX((240 - 100) / 2);
                ImGui::PushFont(font_large);
                if (ImGui::Button("Kapat", ImVec2(100, 30)))
                {
                    show_about = false;
                }
                ImGui::PopFont();
                ImGui::End();
            }
        }

        // Sinif Dongusu Onay Modali
        if (show_reset_confirm)
        {
            ImGui::SetNextWindowPos(ImVec2(30, 80), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(220, 150), ImGuiCond_Always);

            if (ImGui::Begin("Onay", &show_reset_confirm, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
            {
                ImGui::Spacing();
                ImGui::PushFont(font_normal);
                ImGui::TextWrapped("Siniftaki tum ogrenciler secildi! Liste bastan baslatilsin mi?");
                ImGui::PopFont();
                
                ImGui::Spacing();
                ImGui::Spacing();
                
                ImGui::SetCursorPosX((220 - 120) / 2);
                ImGui::PushFont(font_large);
                if (ImGui::Button("Evet", ImVec2(60, 30)))
                {
                    ResetStudentPool(class_size);
                    show_reset_confirm = false;
                }
                ImGui::SameLine();
                if (ImGui::Button("Hayir", ImVec2(60, 30)))
                {
                    show_reset_confirm = false;
                }
                ImGui::PopFont();
                ImGui::End();
            }
        }

        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // SaveData(); // Data kayıt devre dışı

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

#ifdef _WIN32
    //WSACleanup();
#endif

    return 0;
}
