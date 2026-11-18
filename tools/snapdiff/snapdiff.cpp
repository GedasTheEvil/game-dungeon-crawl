// Render snapshots' pixel diff (plan render-snapshots): every PNG of the reference folder against the one of the same
// name in the output folder. Per image: the share of pixels with a channel off by more than the threshold, the largest
// channel error, and a diff PNG (the pixels off in red over a dimmed copy of the new image) for an image that changed.
//
//   snapdiff [--threshold N] [--tolerance PCT] [--diff DIR] REF_DIR OUT_DIR
//
// --threshold: a pixel counts as off when a channel differs by more than N (0-255, default 8).
// --tolerance: an image fails when more than PCT % of its pixels are off (default 0.1: about 230 pixels of 640x360).
// Two runs of one build give identical images, so any pixel off is a change; the tolerance leaves room for a port's
// rounding, not for noise.
// --diff: where the diff PNGs go (default OUT_DIR/diff).
// Exit 0 when every image is within the tolerance, 1 when one is not (or is missing, new, or of another size), 2 on
// a usage or read error.
#include "../../external/stb/stb_image.h"
#include "../../external/stb/stb_image_write.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <print>
#include <set>
#include <string>
#include <vector>

namespace {
namespace fs = std::filesystem;

struct Options {
	int threshold = 8;
	double tolerance = 0.1;
	fs::path refDir, outDir, diffDir;
};

struct Image {
	int width = 0, height = 0;
	std::vector<unsigned char> rgb;
};

bool readPng(const fs::path& path, Image& image) {
	int channels = 0;
	std::unique_ptr<unsigned char, decltype(&stbi_image_free)> data(
		stbi_load(path.c_str(), &image.width, &image.height, &channels, 3),
		stbi_image_free
	);
	if (!data)
		return false;
	image.rgb.assign(data.get(), data.get() + static_cast<size_t>(image.width) * static_cast<size_t>(image.height) * 3);
	return true;
}

struct Diff {
	size_t off = 0; // pixels with a channel off by more than the threshold
	int maxError = 0; // the largest channel error
};

// Compares two images of one size; writes the diff picture into diff (same size, RGB).
Diff compare(const Image& ref, const Image& out, int threshold, std::vector<unsigned char>& diff) {
	Diff result;
	diff.resize(out.rgb.size());
	for (size_t p = 0; p < out.rgb.size(); p += 3) {
		int error = 0;
		for (size_t c = 0; c < 3; c++)
			error = std::max(error, std::abs(int(ref.rgb[p + c]) - int(out.rgb[p + c])));
		result.maxError = std::max(result.maxError, error);
		const bool off = error > threshold;
		if (off)
			result.off++;
		const int grey = (int(out.rgb[p]) + int(out.rgb[p + 1]) + int(out.rgb[p + 2])) / 9; // a third of the mean
		diff[p] = off ? 255 : static_cast<unsigned char>(grey);
		diff[p + 1] = off ? 0 : static_cast<unsigned char>(grey);
		diff[p + 2] = off ? 0 : static_cast<unsigned char>(grey);
	}
	return result;
}

std::set<std::string> pngNames(const fs::path& dir) {
	std::set<std::string> names;
	std::error_code ec;
	for (const fs::directory_entry& entry : fs::directory_iterator(dir, ec))
		if (entry.is_regular_file() && entry.path().extension() == ".png")
			names.insert(entry.path().filename().string());
	return names;
}

bool parseArgs(int argc, char** argv, Options& options) {
	std::vector<std::string> dirs;
	for (int i = 1; i < argc; i++) {
		const std::string arg = argv[i];
		const bool hasValue = i + 1 < argc;
		if (arg == "--threshold" && hasValue)
			options.threshold = std::atoi(argv[++i]);
		else if (arg == "--tolerance" && hasValue)
			options.tolerance = std::atof(argv[++i]);
		else if (arg == "--diff" && hasValue)
			options.diffDir = argv[++i];
		else if (arg.starts_with("--"))
			return false;
		else
			dirs.push_back(arg);
	}
	if (dirs.size() != 2)
		return false;
	options.refDir = dirs[0];
	options.outDir = dirs[1];
	if (options.diffDir.empty())
		options.diffDir = options.outDir / "diff";
	return true;
}

// One image's row of the table; returns whether it passes.
bool checkImage(const Options& options, const std::string& name, bool inRef, bool inOut) {
	if (!inRef) {
		std::println("{:<44} {:>8} {:>4}  NEW (no reference: make snapshots-ref)", name, "-", "-");
		return false;
	}
	if (!inOut) {
		std::println("{:<44} {:>8} {:>4}  MISSING (not taken)", name, "-", "-");
		return false;
	}
	Image ref;
	Image out;
	if (!readPng(options.refDir / name, ref) || !readPng(options.outDir / name, out)) {
		std::println("{:<44} {:>8} {:>4}  UNREADABLE", name, "-", "-");
		return false;
	}
	if (ref.width != out.width || ref.height != out.height) {
		std::println(
			"{:<44} {:>8} {:>4}  SIZE {}x{} was {}x{}",
			name,
			"-",
			"-",
			out.width,
			out.height,
			ref.width,
			ref.height
		);
		return false;
	}
	std::vector<unsigned char> diff;
	const Diff result = compare(ref, out, options.threshold, diff);
	const double share = 100.0 * double(result.off) /
						 double(static_cast<size_t>(out.width) * static_cast<size_t>(out.height));
	const bool pass = share <= options.tolerance;
	std::string note = pass ? "ok" : "FAIL";
	if (result.maxError > 0) {
		const fs::path diffPath = options.diffDir / name;
		stbi_write_png(diffPath.c_str(), out.width, out.height, 3, diff.data(), out.width * 3);
		note += "  " + diffPath.string();
	}
	std::println("{:<44} {:>7.3f}% {:>4}  {}", name, share, result.maxError, note);
	return pass;
}
} // namespace

int main(int argc, char** argv) {
	Options options;
	if (!parseArgs(argc, argv, options)) {
		std::println(stderr, "usage: snapdiff [--threshold N] [--tolerance PCT] [--diff DIR] REF_DIR OUT_DIR");
		return 2;
	}
	const std::set<std::string> refNames = pngNames(options.refDir);
	const std::set<std::string> outNames = pngNames(options.outDir);
	if (refNames.empty() && outNames.empty()) {
		std::println(stderr, "snapdiff: no PNG in {} or {}", options.refDir.string(), options.outDir.string());
		return 2;
	}
	std::error_code ec;
	fs::remove_all(options.diffDir, ec);
	fs::create_directories(options.diffDir, ec);
	std::set<std::string> names = refNames;
	names.insert(outNames.begin(), outNames.end());
	std::println(
		"{:<44} {:>8} {:>4}  (pixels off by more than {}, at most {}% each)",
		"image",
		"off",
		"max",
		options.threshold,
		options.tolerance
	);
	int failed = 0;
	int changed = 0;
	for (const std::string& name : names) {
		if (!checkImage(options, name, refNames.contains(name), outNames.contains(name)))
			failed++;
		if (fs::exists(options.diffDir / name))
			changed++;
	}
	std::println("== snapdiff: {} images, {} changed, {} over the tolerance", names.size(), changed, failed);
	return failed == 0 ? 0 : 1;
}
