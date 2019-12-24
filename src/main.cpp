#include "fmt/format.h"
#include "cli11/cli11.hpp"

#include "core/Log.h"
#include "DataCompression/LosslessCompression.h"
#include "Platform/MemoryMappedFile.h"
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

vector<byte>
BuildCRSM(const unique_ptr<Platform::IMemoryMappedFile> &vertSpirv,
          const unique_ptr<Platform::IMemoryMappedFile> &fragSpirv) {
  vector<byte> uncompressed;

  struct Header {
    uint32_t VertSize{0};
    uint32_t FragSize{0};
  };

  Header header;
  header.VertSize = (uint32_t)vertSpirv->size();
  header.FragSize = (uint32_t)fragSpirv->size();

  uncompressed.resize(sizeof(header) + header.VertSize + header.FragSize);

  copy((byte *)&header, (byte *)&header + sizeof(header), begin(uncompressed));
  copy(vertSpirv->data(), vertSpirv->data() + vertSpirv->size(),
       begin(uncompressed) + sizeof(header));
  copy(fragSpirv->data(), fragSpirv->data() + fragSpirv->size(),
       begin(uncompressed) + sizeof(header) + vertSpirv->size());

  return DataCompression::Compress(data(uncompressed), (uint32_t)size(uncompressed), 18);
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

  auto vertSpirv = Platform::OpenMMapFile(compiledVertPath);
  auto fragSpirv = Platform::OpenMMapFile(compiledFragPath);

  if (!vertSpirv || !fragSpirv || vertSpirv->size() == 0 ||
      fragSpirv->size() == 0) {
    Log::Fail("Failed to load compile spirv for {} and/or {}",
              vertPath.string(), fragPath.string());
  }

  vector<byte> crsm = BuildCRSM(vertSpirv, fragSpirv);

  vertSpirv.reset();
  fragSpirv.reset();
  fs::remove(compiledVertPath);
  fs::remove(compiledFragPath);

  return 0; 
}