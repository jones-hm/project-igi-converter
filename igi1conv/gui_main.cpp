#include "gui_main.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "imgui.h"
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
                // ARGB8888 -> RGBA8888
                for (size_t i = 0; i < img.pixels.size() / 4; ++i) {
                    rgba[i*4 + 0] = img.pixels[i*4 + 1]; // R
                    rgba[i*4 + 1] = img.pixels[i*4 + 2]; // G
                    rgba[i*4 + 2] = img.pixels[i*4 + 3]; // B
                    rgba[i*4 + 3] = img.pixels[i*4 + 0]; // A
                }
            } else if (img.mode == 2) {
                // RGB565 -> RGBA8888
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

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

        ImGui::Begin("Asset Browser");
        ImGui::Text("Current Dir: %s", current_dir.string().c_str());
        if (ImGui::Button("..")) {
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
                    if (min_pt.x > max_pt.x) { min_pt = glm::vec3(-1); max_pt = glm::vec3(1); } // Fallback
                    
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

                        // Backface culling
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
            
            if (ext == ".tex" || ext == ".spr" || ext == ".pic") {
                if (ImGui::Button("Convert to PNG")) {
                    std::string cmd = "igi1conv tex to-png \"" + selected_file.string() + "\"";
                    system(cmd.c_str());
                }
            } else if (ext == ".qvm") {
                if (ImGui::Button("Decompile to QSC")) {
                    std::string cmd = "igi1conv qvm decompile \"" + selected_file.string() + "\"";
                    system(cmd.c_str());
                }
            } else if (ext == ".qsc") {
                if (ImGui::Button("Compile to QVM")) {
                    std::string cmd = "igi1conv qsc compile \"" + selected_file.string() + "\"";
                    system(cmd.c_str());
                }
            } else if (ext == ".mef") {
                if (ImGui::Button("Export to OBJ")) {
                    std::string cmd = "igi1conv mef export \"" + selected_file.string() + "\"";
                    system(cmd.c_str());
                }
            }
            
            ImGui::Separator();
            ImGui::TextWrapped("System commands will be executed in the background. Check CLI output for details.");
        } else {
            ImGui::Text("Select a file to see conversion options.");
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
