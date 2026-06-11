#include "gui_main.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <memory>
#include <array>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>

#include "qvm_parser.h"
#include "qvm_decompiler.h"
#include "tex_parser.h"
#include "mef_native.h"

#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace fs = std::filesystem;

static fs::path current_dir = fs::current_path();
static fs::path selected_file;
static std::string viewer_text;
static GLuint current_texture = 0;
static int current_tex_w = 0, current_tex_h = 0;
static bool show_image = false;

static ParsedGeometry current_mef;
static bool show_mef = false;
static float mef_rot_x = 0.0f;
static float mef_rot_y = 0.0f;
static float mef_zoom = 1.0f;

static std::string console_output;
static char output_dir_buf[512] = "";

// Helper to execute command and return output
static std::string exec(const char* cmd) {
#ifdef _WIN32
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
#else
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
#endif
    if (!pipe) return "Error: Failed to run command!";
    std::array<char, 128> buffer;
    std::string result;
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

static void LoadTexture(const uint8_t* rgba, int w, int h) {
    if (current_texture != 0) {
        glDeleteTextures(1, &current_texture);
        current_texture = 0;
    }
    if (!rgba) return;
    
    glGenTextures(1, &current_texture);
    glBindTexture(GL_TEXTURE_2D, current_texture);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    
    current_tex_w = w;
    current_tex_h = h;
}

static void LoadSelectedFile() {
    viewer_text.clear();
    show_image = false;
    show_mef = false;
    if (current_texture != 0) {
        glDeleteTextures(1, &current_texture);
        current_texture = 0;
    }

    if (!fs::exists(selected_file)) return;
    if (fs::is_directory(selected_file)) return;
    
    std::string ext = selected_file.extension().string();
    for (auto& c : ext) c = tolower(c);
    
    if (ext == ".qvm") {
        QVMFile qvm = QVM_Parse(selected_file.string());
        if (qvm.valid) {
            viewer_text = QVM_DecompileToString(qvm);
            if (viewer_text.empty()) viewer_text = "// Decompilation failed";
        } else {
            viewer_text = "Failed to parse QVM:\n" + qvm.error;
        }
    } else if (ext == ".qsc" || ext == ".txt" || ext == ".json" || ext == ".md" || ext == ".h" || ext == ".cpp") {
        std::ifstream ifs(selected_file, std::ios::binary);
        if (ifs) {
            std::ostringstream ss;
            ss << ifs.rdbuf();
            viewer_text = ss.str();
        }
    } else if (ext == ".tex" || ext == ".spr" || ext == ".pic") {
        TEXFile tex = TEX_Parse(selected_file.string());
        if (tex.valid && !tex.images.empty()) {
            std::vector<uint8_t> rgba(tex.images[0].width * tex.images[0].height * 4, 255);
            const auto& img = tex.images[0];
            if (img.mode == 3) {
                for (size_t i = 0; i < img.pixels.size() / 4; ++i) {
                    rgba[i*4 + 0] = img.pixels[i*4 + 1];
                    rgba[i*4 + 1] = img.pixels[i*4 + 2];
                    rgba[i*4 + 2] = img.pixels[i*4 + 3];
                    rgba[i*4 + 3] = img.pixels[i*4 + 0];
                }
            } else if (img.mode == 2) {
                for (size_t i = 0; i < img.pixels.size() / 2; ++i) {
                    uint16_t p = (img.pixels[i*2+1] << 8) | img.pixels[i*2];
                    rgba[i*4 + 0] = ((p >> 11) & 0x1F) * 255 / 31;
                    rgba[i*4 + 1] = ((p >> 5) & 0x3F) * 255 / 63;
                    rgba[i*4 + 2] = (p & 0x1F) * 255 / 31;
                    rgba[i*4 + 3] = 255;
                }
            }
            LoadTexture(rgba.data(), img.width, img.height);
            show_image = true;
            viewer_text = "Image Loaded: " + std::to_string(img.width) + "x" + std::to_string(img.height);
        } else {
            viewer_text = "Failed to parse image:\n" + tex.error;
        }
    } else if (ext == ".mef") {
        try {
            current_mef = ParseMefFile(selected_file.string());
            show_mef = true;
            viewer_text = "MEF Loaded: " + std::to_string(current_mef.vertices.size()) + " vertices, " + 
                          std::to_string(current_mef.triangles.size()) + " triangles.";
            mef_rot_x = 0.0f;
            mef_rot_y = 0.0f;
            mef_zoom = 1.0f;
        } catch (std::exception& e) {
            viewer_text = std::string("Failed to parse MEF:\n") + e.what();
        }
    } else {
        viewer_text = "Preview not supported for " + ext + "\n(Binary or unknown format)";
    }
}

static void glfw_error_callback(int error, const char* description)
{
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

static void ApplyModernTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;
    
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]                   = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.11f, 0.11f, 0.14f, 0.92f);
    colors[ImGuiCol_Border]                 = ImVec4(0.25f, 0.25f, 0.27f, 0.50f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.30f, 0.30f, 0.33f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    colors[ImGuiCol_Button]                 = ImVec4(0.22f, 0.22f, 0.25f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.28f, 0.28f, 0.32f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.35f, 0.35f, 0.38f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.22f, 0.22f, 0.25f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.28f, 0.28f, 0.32f, 1.00f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.35f, 0.35f, 0.38f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.22f, 0.22f, 0.25f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.28f, 0.28f, 0.32f, 1.00f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.35f, 0.35f, 0.38f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.22f, 0.22f, 0.25f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.28f, 0.28f, 0.32f, 1.00f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.35f, 0.35f, 0.38f, 1.00f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.28f, 0.28f, 0.32f, 1.00f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.22f, 0.22f, 0.25f, 1.00f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_DockingPreview]         = ImVec4(0.33f, 0.67f, 0.86f, 0.70f);
    colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
}

int run_gui()
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "IGI Game Asset Viewer & Converter", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    if (!font) {
        font = io.Fonts->AddFontDefault();
    }

    ApplyModernTheme();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("MainDockSpace", nullptr, window_flags);
        ImGui::PopStyleVar();
        ImGui::PopStyleVar(2);

        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        
        static bool first_time = true;
        if (first_time) {
            first_time = false;
            ImGui::DockBuilderRemoveNode(dockspace_id); 
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

            ImGuiID dock_main_id = dockspace_id;
            ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.25f, nullptr, &dock_main_id);
            ImGuiID dock_id_bottom_left = ImGui::DockBuilderSplitNode(dock_id_left, ImGuiDir_Down, 0.30f, nullptr, &dock_id_left);

            ImGui::DockBuilderDockWindow("Asset Browser", dock_id_left);
            ImGui::DockBuilderDockWindow("Convert", dock_id_bottom_left);
            ImGui::DockBuilderDockWindow("Viewer", dock_main_id);
            ImGui::DockBuilderFinish(dockspace_id);
        }
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
        ImGui::End();

        ImGui::Begin("Asset Browser");
        ImGui::Text("Current Dir: %s", current_dir.string().c_str());
        if (ImGui::Button("..", ImVec2(50, 0))) {
            current_dir = current_dir.parent_path();
        }
        ImGui::Separator();
        
        ImGui::BeginChild("Files");
        try {
            for (const auto& entry : fs::directory_iterator(current_dir)) {
                bool is_dir = entry.is_directory();
                std::string name = entry.path().filename().string();
                if (is_dir) name = "[DIR] " + name;
                
                if (ImGui::Selectable(name.c_str(), selected_file == entry.path())) {
                    if (is_dir) {
                        current_dir = entry.path();
                    } else {
                        selected_file = entry.path();
                        LoadSelectedFile();
                    }
                }
                
                if (ImGui::BeginPopupContextItem(name.c_str())) {
                    if (ImGui::MenuItem("View / Load")) {
                        if (is_dir) {
                            current_dir = entry.path();
                        } else {
                            selected_file = entry.path();
                            LoadSelectedFile();
                        }
                    }
                    if (!is_dir) {
                        if (ImGui::MenuItem("Open Convert Options")) {
                            selected_file = entry.path();
                            LoadSelectedFile();
                            ImGui::SetWindowFocus("Convert");
                        }
                    }
                    ImGui::EndPopup();
                }
            }
        } catch (...) {}
        ImGui::EndChild();
        ImGui::End();

        ImGui::Begin("Viewer");
        if (!selected_file.empty()) {
            ImGui::Text("File: %s", selected_file.string().c_str());
            ImGui::Separator();
            
            if (show_mef) {
                ImVec2 avail = ImGui::GetContentRegionAvail();
                if (avail.x > 0 && avail.y > 0) {
                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    ImDrawList* draw_list = ImGui::GetWindowDrawList();
                    
                    ImGui::InvisibleButton("##mef_view", avail);
                    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                        ImVec2 delta = ImGui::GetIO().MouseDelta;
                        mef_rot_y += delta.x * 0.01f;
                        mef_rot_x += delta.y * 0.01f;
                    }
                    if (ImGui::IsItemHovered()) {
                        mef_zoom += ImGui::GetIO().MouseWheel * 0.1f;
                        if (mef_zoom < 0.1f) mef_zoom = 0.1f;
                    }

                    glm::vec3 min_pt(1e9f), max_pt(-1e9f);
                    for (auto& v : current_mef.vertices) {
                        min_pt = glm::min(min_pt, v.pos);
                        max_pt = glm::max(max_pt, v.pos);
                    }
                    if (min_pt.x > max_pt.x) { min_pt = glm::vec3(-1); max_pt = glm::vec3(1); }
                    
                    glm::vec3 center = (min_pt + max_pt) * 0.5f;
                    float size = glm::length(max_pt - min_pt);
                    if (size < 0.001f) size = 1.0f;

                    glm::mat4 proj = glm::perspective(glm::radians(45.0f), avail.x / avail.y, 0.1f, 10000.0f);
                    glm::mat4 view = glm::lookAt(glm::vec3(0, 0, size * 1.5f / mef_zoom), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
                    
                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::rotate(model, mef_rot_x, glm::vec3(1, 0, 0));
                    model = glm::rotate(model, mef_rot_y, glm::vec3(0, 1, 0));
                    model = glm::translate(model, -center);

                    glm::mat4 mvp = proj * view * model;

                    draw_list->AddRectFilled(pos, ImVec2(pos.x + avail.x, pos.y + avail.y), IM_COL32(30, 30, 30, 255));

                    int drawn_tris = 0;
                    for (auto& tri : current_mef.triangles) {
                        glm::vec4 p1 = mvp * glm::vec4(current_mef.vertices[tri[0]].pos, 1.0f);
                        glm::vec4 p2 = mvp * glm::vec4(current_mef.vertices[tri[1]].pos, 1.0f);
                        glm::vec4 p3 = mvp * glm::vec4(current_mef.vertices[tri[2]].pos, 1.0f);

                        if (p1.w <= 0 || p2.w <= 0 || p3.w <= 0) continue; 

                        p1 /= p1.w; p2 /= p2.w; p3 /= p3.w;

                        ImVec2 sp1 = ImVec2(pos.x + (p1.x + 1.0f) * 0.5f * avail.x, pos.y + (1.0f - p1.y) * 0.5f * avail.y);
                        ImVec2 sp2 = ImVec2(pos.x + (p2.x + 1.0f) * 0.5f * avail.x, pos.y + (1.0f - p2.y) * 0.5f * avail.y);
                        ImVec2 sp3 = ImVec2(pos.x + (p3.x + 1.0f) * 0.5f * avail.x, pos.y + (1.0f - p3.y) * 0.5f * avail.y);

                        float area = (sp2.x - sp1.x) * (sp3.y - sp1.y) - (sp3.x - sp1.x) * (sp2.y - sp1.y);
                        if (area > 0) {
                            draw_list->AddTriangle(sp1, sp2, sp3, IM_COL32(0, 255, 0, 128));
                            drawn_tris++;
                        }
                    }
                    ImGui::SetCursorScreenPos(pos);
                    ImGui::Text("Rendered %d / %d triangles", drawn_tris, (int)current_mef.triangles.size());
                }
            } else if (show_image && current_texture != 0) {
                ImGui::Image((void*)(intptr_t)current_texture, ImVec2(current_tex_w, current_tex_h));
            } else {
                ImGui::InputTextMultiline("##source", &viewer_text[0], viewer_text.capacity(), ImVec2(-1.0f, -1.0f), ImGuiInputTextFlags_ReadOnly);
            }
        } else {
            ImGui::Text("No file selected.");
        }
        ImGui::End();
        
        ImGui::Begin("Convert");
        if (!selected_file.empty()) {
            std::string ext = selected_file.extension().string();
            for (auto& c : ext) c = tolower(c);
            
            ImGui::Text("File:");
            ImGui::TextWrapped("%s", selected_file.string().c_str());
            ImGui::Separator();
            
            ImGui::Text("Output Directory / File (Optional):");
            ImGui::InputText("##outdir", output_dir_buf, sizeof(output_dir_buf));
            std::string out_arg = std::string(output_dir_buf);
            
            ImGui::Spacing();
            
            if (ext == ".tex" || ext == ".spr" || ext == ".pic") {
                if (ImGui::Button("Convert to PNG", ImVec2(-1, 0))) {
                    std::string cmd = "igi1conv tex to-png \"" + selected_file.string() + "\"";
                    if (!out_arg.empty()) cmd += " -o \"" + out_arg + "\"";
                    console_output = exec(cmd.c_str());
                }
                if (ImGui::Button("Convert to TGA", ImVec2(-1, 0))) {
                    std::string cmd = "igi1conv tex to-tga \"" + selected_file.string() + "\"";
                    if (!out_arg.empty()) cmd += " -o \"" + out_arg + "\"";
                    console_output = exec(cmd.c_str());
                }
            } else if (ext == ".qvm") {
                if (ImGui::Button("Decompile to QSC", ImVec2(-1, 0))) {
                    std::string cmd = "igi1conv qvm decompile \"" + selected_file.string() + "\"";
                    if (!out_arg.empty()) cmd += " -o \"" + out_arg + "\"";
                    console_output = exec(cmd.c_str());
                }
            } else if (ext == ".qsc") {
                if (ImGui::Button("Compile to QVM", ImVec2(-1, 0))) {
                    std::string cmd = "igi1conv qsc compile \"" + selected_file.string() + "\"";
                    if (!out_arg.empty()) cmd += " -o \"" + out_arg + "\"";
                    console_output = exec(cmd.c_str());
                }
            } else if (ext == ".mef") {
                if (ImGui::Button("Export to OBJ", ImVec2(-1, 0))) {
                    std::string cmd = "igi1conv mef export \"" + selected_file.string() + "\"";
                    if (!out_arg.empty()) cmd += " -o \"" + out_arg + "\"";
                    console_output = exec(cmd.c_str());
                }
            } else if (ext == ".res") {
                if (ImGui::Button("Extract RES", ImVec2(-1, 0))) {
                    std::string cmd = "igi1conv res extract \"" + selected_file.string() + "\"";
                    if (!out_arg.empty()) cmd += " -o \"" + out_arg + "\"";
                    console_output = exec(cmd.c_str());
                }
            } else if (ext == ".dat") {
                if (ImGui::Button("Dump DAT Info", ImVec2(-1, 0))) {
                    std::string cmd = "igi1conv dat info \"" + selected_file.string() + "\"";
                    console_output = exec(cmd.c_str());
                }
            } else {
                ImGui::Text("No specific conversions for this format.");
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Console Output:");
            ImGui::BeginChild("Console", ImVec2(-1, -1), true);
            ImGui::TextUnformatted(console_output.c_str());
            ImGui::EndChild();
        } else {
            ImGui::TextWrapped("Select an asset to view conversion and extraction options.");
        }
        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }

    if (current_texture != 0) {
        glDeleteTextures(1, &current_texture);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
