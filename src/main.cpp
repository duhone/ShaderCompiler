#include "fmt/format.h"
#include "cli11/cli11.hpp"

#include "core/Log.h"
#include "Platform/PathUtils.h"
#include "Platform/Process.h"

#include <cstdio>
#include <chrono>

using namespace std;
using namespace std::chrono_literals;
namespace fs = std::filesystem;
using namespace CR;
using namespace CR::Core;

fs::path CompileShader(const fs::path &input) { 
  char tempFile[L_tmpnam_s];
  tmpnam_s(tempFile);
  fs::path tempPath = fs::temp_directory_path() /= tempFile;

  string cliArgs = fmt::format("{} -o {}", input.string(), tempPath.string());

  auto glslc = Platform::CRCreateProcess("glslc.exe", cliArgs.c_str());
  if (!glslc->WaitForClose(60s)) {
    Log::Fail("glslc shader compiler did not complete after 60s");
  }
  auto exitCode = glslc->GetExitCode();

  if (!exitCode.has_value() || exitCode.value() != 0) {
    Log::Fail("failed to compile shader {}", input.string());
  }

  return tempPath;
}

int main(int argc, char **argv) { 
  CLI::App app{"Shader Compiler"};
  string vertFileName = "";
  string fragFileName = "";
  string outputFileName = "";
  app.add_option("-v,--vert", vertFileName,
                 "Input Vertex Shader. Must have .vert extension")->required();
  app.add_option("-f,--frag", fragFileName,
                     "Input Fragment Shader. Must have .frag extension")->required();
  app.add_option("-o,--output", outputFileName,
                                     "Output file. Must have .crsm extension")->required();

  CLI11_PARSE(app, argc, argv);

  fs::path vertPath{vertFileName};
  fs::path fragPath{fragFileName};
  fs::path outputPath{outputFileName};

  if (!vertPath.has_extension() || vertPath.extension() != ".vert") {
    CLI::Error error{"extension",
                     "Vertex Shader requires .vert file name extension", CLI::ExitCodes::FileError};
    app.exit(error);
  }
  if (!fragPath.has_extension() || fragPath.extension() != ".frag") {
    CLI::Error error{"extension",
                     "Fragment Shader requires .frag file name extension",
                     CLI::ExitCodes::FileError};
    app.exit(error);
  }
  if (!outputPath.has_extension() || outputPath.extension() != ".crsm") {
    CLI::Error error{"extension",
                     "Output file requires .crsm file name extension",
                     CLI::ExitCodes::FileError};
    app.exit(error);
  }

  vertPath = Platform::GetCurrentProcessPath() / vertPath;
  fragPath = Platform::GetCurrentProcessPath() / fragPath;
  outputPath = Platform::GetCurrentProcessPath() / outputPath;

  fs::path compiledVertPath = CompileShader(vertPath);
  fs::path compiledFragPath = CompileShader(fragPath);

  fs::remove(compiledVertPath);
  fs::remove(compiledFragPath);

  return 0; 
}