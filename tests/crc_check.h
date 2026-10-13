#ifndef CRC_CHECK_H
#define CRC_CHECK_H

// Checks the integrity of every table file of a folder: each file is decompressed and
// the CRC-32 of its data compared with the CRC stored in the gzip trailer. Files are
// checked in parallel, 20 per output line.

#include "tb/egtb/tb_reader.h"
extern "C" {
#include "tb/egtb/dictzip/dz.h"
}
#include <zlib.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>


// Empty if the file is OK, otherwise the error
inline std::string check_file_crc(const std::filesystem::path& path)
{
	void* header = dz_open(path.generic_string().c_str());
	if (!dz_is_open(header))
		return "can't open";
	const unsigned long length = dz_get_orig_length(header);
	const unsigned long stored_crc = dz_get_orig_crc(header);
	constexpr unsigned long BUFFER_SIZE = 4 << 20;
	std::vector<char> buffer(BUFFER_SIZE);
	unsigned long crc = crc32(0L, Z_NULL, 0);
	std::string error;
	for (unsigned long pos = 0; pos < length; pos += BUFFER_SIZE)
	{
		const unsigned long size = std::min(BUFFER_SIZE, length - pos);
		if (dz_read(header, pos, size, buffer.data()) != 0)
		{
			error = "can't decompress the data at " + std::to_string(pos);
			break;
		}
		crc = crc32(crc, reinterpret_cast<const Bytef*>(buffer.data()), static_cast<uInt>(size));
	}
	dz_close(header);
	if (error.empty() && crc != stored_crc)
	{
		std::stringstream ss;
		ss << "CRC 0x" << std::hex << std::setw(8) << std::setfill('0') << crc
		   << " != stored 0x" << std::setw(8) << stored_crc;
		error = ss.str();
	}
	return error;
}

// Returns 0 if all files are OK
inline int crc_tests(const std::string& egtb_path)
{
	constexpr size_t FILES_PER_LINE = 20;
	const std::string& ext = VariantTb::TB_Reader::file_extension_compressed;
	std::vector<std::filesystem::path> files;
	for (const auto& entry : std::filesystem::directory_iterator(egtb_path))
		if (entry.is_regular_file() && entry.path().extension() == ext)
			files.push_back(entry.path());
	std::sort(files.begin(), files.end());
	if (files.empty())
	{
		std::cout << "CRC: no " << ext << " files" << std::endl;
		return -112;
	}

	const auto start = std::chrono::steady_clock::now();
	const size_t num_threads = std::max(1u, std::min(8u, std::thread::hardware_concurrency()));
	size_t num_errors = 0;
	for (size_t first = 0; first < files.size(); first += FILES_PER_LINE)
	{
		const size_t last = std::min(files.size(), first + FILES_PER_LINE);
		std::vector<std::string> errors(last - first);
		std::atomic<size_t> next{ first };
		std::vector<std::thread> threads;
		for (size_t t = 0; t < std::min(num_threads, last - first); t++)
			threads.emplace_back([&]() {
				for (size_t i; (i = next++) < last; )
					errors[i - first] = check_file_crc(files[i]);
			});
		for (auto& th : threads)
			th.join();

		size_t line_errors = 0;
		for (const auto& e : errors)
			line_errors += !e.empty();
		num_errors += line_errors;
		// Same columns as the "Test <FEN>: ..." lines: "CRC  " + 40 characters + ": "
		std::stringstream label;
		label << std::setw(4) << std::right << first + 1 << "-" << std::setw(4) << std::left << last << " "
		      << std::setw(12) << files[first].filename().string() << ".. " << files[last - 1].filename().string();
		std::cout << "CRC  " << std::setw(40) << std::left << label.str() << ": "
		          << (line_errors ? std::to_string(line_errors) + " ERROR(S)" : "PASSED") << std::endl;
		for (size_t i = first; i < last; i++)
			if (!errors[i - first].empty())
				std::cout << "     " << std::setw(40) << std::left << files[i].filename().string() << ": "
				          << errors[i - first] << " --> ERROR" << std::endl;
	}
	const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
	std::stringstream time;
	time << std::fixed << std::setprecision(1) << seconds << " s";
	std::cout << "CRC  " << std::setw(40) << std::left << (std::to_string(files.size()) + " files") << ": "
	          << (num_errors ? std::to_string(num_errors) + " ERROR(S)" : "PASSED") << " (" << time.str() << ")" << std::endl;
	return num_errors ? -112 : 0;
}

#endif
