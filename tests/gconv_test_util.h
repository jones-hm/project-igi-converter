// gconv_test_util.h — self-contained helpers for the gconv CLI test suite.
//
// The suite spawns the freshly built gconv.exe (next to gconv_tests.exe) and
// drives it against a corpus of real IGI game files.  The corpus directory is
// taken from the GCONV_TEST_CORPUS environment variable, defaulting to
// D:\IGI1\full_test.  Tests that need a file which is absent are SKIPPED (not
// failed) so the suite stays green on machines without the corpus.
#pragma once

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace gconv_test {

// Directory of the running test executable (gconv.exe lives alongside it).
inline std::string ExeDir() {
    char buf[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    std::filesystem::path p(buf);
    return p.parent_path().string();
}

inline std::string GConvExe() {
    return ExeDir() + "\\gconv.exe";
}

// Root of the test corpus (env override, else the canonical full_test dir).
inline std::string CorpusDir() {
    char* env = nullptr;
    size_t len = 0;
    if (_dupenv_s(&env, &len, "GCONV_TEST_CORPUS") == 0 && env) {
        std::string v(env);
        free(env);
        if (!v.empty()) return v;
    }
    return "D:\\IGI1\\full_test";
}

inline std::string Corpus(const std::string& rel) {
    return CorpusDir() + "\\" + rel;
}

// Temp directory scoped to a single test, auto-removed on destruction.
class TempDir {
public:
    TempDir() {
        char buf[MAX_PATH];
        GetTempPathA(MAX_PATH, buf);
        static unsigned counter = 0;
        path_ = std::string(buf) + "gconv_test_" +
                std::to_string(GetCurrentProcessId()) + "_" +
                std::to_string(++counter);
        std::filesystem::create_directories(path_);
    }
    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }
    std::string operator/(const std::string& name) const { return path_ + "\\" + name; }
    const std::string& str() const { return path_; }
private:
    std::string path_;
};

// Run gconv.exe with the given (already-quoted where needed) argument string.
// If captureOut is non-null, the child's stdout+stderr are written there.
// Returns the process exit code, or -1 if the process could not be started.
inline int RunGConv(const std::string& args, std::string* captureOut = nullptr,
                    DWORD timeoutMs = 30000) {
    const std::string exePath = GConvExe();
    std::string cmdLine = "\"" + exePath + "\" " + args;

    SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };

    // Redirect child output to a temp file (file, not pipe, to avoid the
    // classic full-buffer deadlock without a reader thread).
    std::string outFile;
    HANDLE hOut = INVALID_HANDLE_VALUE;
    if (captureOut) {
        char tmp[MAX_PATH], name[MAX_PATH];
        GetTempPathA(MAX_PATH, tmp);
        GetTempFileNameA(tmp, "gcv", 0, name);
        outFile = name;
        hOut = CreateFileA(outFile.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                           &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
    } else {
        hOut = CreateFileA("NUL", GENERIC_WRITE, FILE_SHARE_WRITE,
                           &sa, OPEN_EXISTING, 0, nullptr);
    }

    STARTUPINFOA si = {};
    si.cb          = sizeof(si);
    si.dwFlags     = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput  = hOut;
    si.hStdError   = hOut;
    si.hStdInput   = GetStdHandle(STD_INPUT_HANDLE);

    PROCESS_INFORMATION pi = {};
    std::vector<char> buf(cmdLine.begin(), cmdLine.end());
    buf.push_back('\0');

    BOOL ok = CreateProcessA(nullptr, buf.data(), nullptr, nullptr, TRUE,
                             CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    if (hOut != INVALID_HANDLE_VALUE) CloseHandle(hOut);
    if (!ok) {
        if (!outFile.empty()) DeleteFileA(outFile.c_str());
        return -1;
    }

    DWORD wait = WaitForSingleObject(pi.hProcess, timeoutMs);
    DWORD code = 1;
    if (wait == WAIT_OBJECT_0)
        GetExitCodeProcess(pi.hProcess, &code);
    else
        TerminateProcess(pi.hProcess, 1);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (captureOut && !outFile.empty()) {
        std::ifstream f(outFile, std::ios::binary);
        captureOut->assign(std::istreambuf_iterator<char>(f),
                           std::istreambuf_iterator<char>());
        f.close();
        DeleteFileA(outFile.c_str());
    }
    return static_cast<int>(code);
}

inline std::string Q(const std::string& s) { return "\"" + s + "\""; }

inline bool NonEmptyFile(const std::string& p) {
    std::error_code ec;
    return std::filesystem::exists(p) && std::filesystem::file_size(p, ec) > 0;
}

// Base fixture: ensures gconv.exe exists.
class GConvTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(std::filesystem::exists(GConvExe()))
            << "gconv.exe not found next to test exe: " << GConvExe();
    }
};

} // namespace gconv_test

// Declare a corpus path and SKIP the current test if the file is absent.
// Used in a (void) test body so GTEST_SKIP returns from the test correctly.
#define GCONV_NEED(var, rel)                                            \
    std::string var = ::gconv_test::Corpus(rel);                        \
    if (!std::filesystem::exists(var))                                  \
        GTEST_SKIP() << "corpus file missing: " << var                 \
                     << " (set GCONV_TEST_CORPUS)"
