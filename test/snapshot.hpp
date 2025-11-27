#pragma once 

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <cassert>

namespace snapshot {

inline std::string loadFile(const std::string &path) {
    std::ifstream in(path);
    if (!in) return "";
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

inline void writeFile(const std::string &path, const std::string &data) {
    std::ofstream out(path);
    out << data;
}

inline void ensureDirectory(const std::string &path) {
    std::filesystem::create_directories(path);
}

inline void printDiff(const std::string &expected, const std::string &output) {
    std::istringstream a(expected);
    std::istringstream b(output);

    std::string lineA, lineB;
    unsigned line = 1;

    std::cerr << "--- DIFF (only differing lines shown) ---\n";

    while (true) {
        bool a_ok = static_cast<bool>(std::getline(a, lineA));
        bool b_ok = static_cast<bool>(std::getline(b, lineB));

        if (!a_ok && !b_ok)
            break;  // done

        if (!a_ok || !b_ok || lineA != lineB) {
            std::cerr << "Line " << line << " length:"
                << lineA.length() << " vs " << lineB.length() << "\n"
                << "Expected: " << (a_ok ? lineA : "<EOF>") << "\n"
                << "     Got: " << (b_ok ? lineB : "<EOF>") << "\n";
        }

        line++;
    }

    std::cerr << "-----------------------------------------\n";
}

enum class Res {Pass, Fail, Created};

inline Res test(std::string snapshotPath, const std::string &output,
                bool yesAssert = false) {

  snapshotPath = "test/snapshots/" + snapshotPath;
  auto dir = std::filesystem::path(snapshotPath).parent_path();
  if (!dir.empty())
    ensureDirectory(dir.string());

  // If snapshot doesn't exist, create it
  if (!std::filesystem::exists(snapshotPath)) {
    std::cout << "[SNAPSHOT] Creating snapshot: " << snapshotPath << "\n";
    writeFile(snapshotPath, output);
    return Res::Created;
  }

  std::string expected = loadFile(snapshotPath);
  if (expected != output) {
    std::cerr << "[KO : SNAPSHOT MISMATCH] " << snapshotPath << "\n";
    printDiff(expected, output);
    if (yesAssert)
      assert(false && "Snapshot mismatch");
    return Res::Fail;
  }
  return Res::Pass;
}
}
