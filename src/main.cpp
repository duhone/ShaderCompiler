#include "cli11/cli11.hpp"

#include "Platform/PathUtils.h"

using namespace std;
namespace fs = std::filesystem;
using namespace CR;

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

  return 0; 
}