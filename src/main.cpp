#include "DataCompression/LosslessCompression.h"
#include "Platform/MemoryMappedFile.h"
#include "Platform/PathUtils.h"
#include "Platform/Process.h"
#include "core/BinaryStream.h"
#include "core/FileHandle.h"
#include "core/Log.h"
#include "core/Span.h"
#include "core/storage_buffer.h"

#include <3rdParty/cli11.h>
#include <3rdParty/fmt.h>

#define DOCTEST_CONFIG_IMPLEMENT
#include <3rdParty/doctest.h>

#include <chrono>
#include <cstdio>

using namespace std;
using namespace std::chrono_literals;
namespace fs = std::filesystem;
using namespace CR;
using namespace CR::Core;

fs::path CompileShader(const fs::path& input) {
	char tempFile[L_tmpnam_s];
	tmpnam_s(tempFile);
	fs::path tempPath = fs::temp_directory_path() /= tempFile;

	string cliArgs = fmt::format("{} -o {}", input.string(), tempPath.string());

	Platform::Process glslc("glslc.exe", cliArgs.c_str());
	if(!glslc.WaitForClose(60s)) { Log::Error("glslc shader compiler did not complete after 60s"); }
	auto exitCode = glslc.GetExitCode();

	if(!exitCode.has_value() || exitCode.value() != 0) { Log::Error("failed to compile shader {}", input.string()); }

	return tempPath;
}

void BuildCRSM(Core::FileHandle& a_file, const Platform::MemoryMappedFile& vertSpirv,
               const Platform::MemoryMappedFile& fragSpirv) {
#pragma pack(1)
	struct Header {
		uint32_t FourCC{'CRSM'};
		uint16_t Version{1};
		uint16_t VertSize{0};
		uint16_t FragSize{0};
	};
#pragma pack()

	Core::Log::Require(vertSpirv.size() < std::numeric_limits<uint16_t>::max(),
	                   "shaders must be smaller than 64K currently");
	Core::Log::Require(fragSpirv.size() < std::numeric_limits<uint16_t>::max(),
	                   "shaders must be smaller than 64K currently");

	Header header;
	header.VertSize = (uint16_t)vertSpirv.size();
	header.FragSize = (uint16_t)fragSpirv.size();

	Core::Write(a_file, header);

	Core::storage_buffer<byte> compVert = DataCompression::Compress(Core::Span(vertSpirv.data(), vertSpirv.size()), 18);
	Core::storage_buffer<byte> compFrag = DataCompression::Compress(Core::Span(fragSpirv.data(), fragSpirv.size()), 18);

	Core::Write(a_file, compVert);
	Core::Write(a_file, compFrag);
}

int main(int argc, char** argv) {
	CLI::App app{"Shader Compiler"};
	string vertFileName   = "";
	string fragFileName   = "";
	string outputFileName = "";
	app.add_option("-v,--vert", vertFileName,
	               "Input Vertex Shader. File must have .vert extension, can leave off on command line though")
	    ->required();
	app.add_option("-f,--frag", fragFileName,
	               "Input Fragment Shader. File must have .frag extension, can leave off on command line though")
	    ->required();
	app.add_option("-o,--output", outputFileName,
	               "Output file. File must have .crsm extension, can leave off on command line though")
	    ->required();

	CLI11_PARSE(app, argc, argv);

	fs::path vertPath{vertFileName};
	fs::path fragPath{fragFileName};
	fs::path outputPath{outputFileName};

	if(!vertPath.has_extension()) { vertPath.replace_extension(".vert"); }
	if(!fragPath.has_extension()) { fragPath.replace_extension(".frag"); }
	if(!outputPath.has_extension()) { outputPath.replace_extension(".crsm"); }

	if(!vertPath.has_extension() || vertPath.extension() != ".vert") {
		CLI::Error error{"extension", "Vertex Shader requires .vert file name extension", CLI::ExitCodes::FileError};
		app.exit(error);
	}
	if(!fragPath.has_extension() || fragPath.extension() != ".frag") {
		CLI::Error error{"extension", "Fragment Shader requires .frag file name extension", CLI::ExitCodes::FileError};
		app.exit(error);
	}
	if(!outputPath.has_extension() || outputPath.extension() != ".crsm") {
		CLI::Error error{"extension", "Output file requires .crsm file name extension", CLI::ExitCodes::FileError};
		app.exit(error);
	}

	if(fs::exists(outputPath) && (fs::last_write_time(outputPath) > fs::last_write_time(vertPath) &&
	                              fs::last_write_time(outputPath) > fs::last_write_time(fragPath))) {
		return 0;    // nothing to do
	}

	filesystem::current_path(Platform::GetCurrentProcessPath());

	fs::path compiledVertPath = CompileShader(vertPath);
	fs::path compiledFragPath = CompileShader(fragPath);

	fs::path outputFolder = outputPath;
	outputFolder.remove_filename();
	fs::create_directories(outputFolder);

	{
		Platform::MemoryMappedFile vertSpirv(compiledVertPath);
		Platform::MemoryMappedFile fragSpirv(compiledFragPath);

		if(vertSpirv.size() == 0 || fragSpirv.size() == 0) {
			Log::Error("Failed to load compile spirv for {} and/or {}", vertPath.string(), fragPath.string());
		}

		Core::FileHandle outputFile(outputPath);
		BuildCRSM(outputFile, vertSpirv, fragSpirv);
	}
	fs::remove(compiledVertPath);
	fs::remove(compiledFragPath);

	return 0;
}
