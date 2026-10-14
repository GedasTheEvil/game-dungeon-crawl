#include "paths.h"
#include <filesystem>
#include <system_error>

namespace fs = std::filesystem;

namespace {
fs::path launchDir; // the working directory before enterRootDir(); empty when it did not move
}

std::string Paths::rootDir() {
	std::error_code error;
	const fs::path exe = fs::read_symlink("/proc/self/exe", error);
	if (error || exe.parent_path().filename() != "bin")
		return "";
	return exe.parent_path().parent_path().string();
}

void Paths::enterRootDir() {
	const std::string root = rootDir();
	std::error_code error;
	const fs::path start = fs::current_path(error);
	if (root.empty() || error || fs::equivalent(start, root, error))
		return;
	fs::current_path(root, error);
	if (!error)
		launchDir = start;
}

std::string Paths::fromLaunchDir(const std::string& path) {
	const fs::path p(path);
	if (launchDir.empty() || p.is_absolute() || fs::exists(p))
		return path;
	const fs::path fromStart = launchDir / p;
	return fs::exists(fromStart) ? fromStart.string() : path;
}
