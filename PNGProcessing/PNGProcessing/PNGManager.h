#pragma once
#include <iostream>
#include <filesystem>
#include <unordered_set>
#include <thread>
#include <functional>
#include "PNGMethod.h"

class PNGManager
{
	using path = std::filesystem::path;
public:
	PNGManager(const path from, const path to);
	
	void RunBlur();
	void RunEdge();
	void RunBright(unsigned char thresold);
	void RunAll(unsigned char thresold);

private:
	const path from, to;
	std::vector<path> pngFilePaths;
	static const inline unsigned nCPUs = std::max(std::thread::hardware_concurrency(), 1u);

	static bool is_png(const path& p);
	static std::string rename(const path& filename, const std::string& adder);

	template<class _Op, class... _Valty>
	void multi_threads_working(const PNGBuffer& in, PNGBuffer& out, _Valty&&... _Val);
};
