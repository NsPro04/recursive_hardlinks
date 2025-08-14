#include <iostream>
#include <filesystem>
#include <list>
#include <chrono>

#include "portable-file-dialogs.h"
#include <Windows.h>

void select_and_copy() {
	using pfd::internal::str2wstr;
	using pfd::internal::wstr2str;

	std::filesystem::path source_dir = L"source";
	std::filesystem::path destination_dir = L"destination";

	for (auto* const dir : {&source_dir, &destination_dir}) {
		const std::string dir_type = wstr2str(*dir);
		std::cout << "Select the " << dir_type << " directory." << std::endl;

		const std::string selected_path_str = pfd::select_folder("Select the " + dir_type + " directory.").result();
		std::cout << "selected_path_str: " << selected_path_str << std::endl;

		const std::wstring selected_path_wstr = str2wstr(selected_path_str);
		const std::filesystem::path selected_path = selected_path_wstr;

		if (!std::filesystem::exists(selected_path)) {
			std::cout << "Error: selected_path does not exist." << std::endl;
			return;
		};
		if (!std::filesystem::is_directory(selected_path)) {
			std::cout << "Error: selected_path is not a directory." << std::endl;
			return;
		};

		const std::filesystem::path canonical_selected_path = std::filesystem::canonical(selected_path).lexically_normal();
		std::cout << "canonical_selected_path: " << wstr2str(canonical_selected_path) << std::endl;

		if (!std::filesystem::exists(canonical_selected_path)) {
			std::cout << "Error: canonical_selected_path does not exist." << std::endl;
			return;
		};
		if (!std::filesystem::is_directory(canonical_selected_path)) {
			std::cout << "Error: canonical_selected_path is not a directory." << std::endl;
			return;
		};

		dir->assign(canonical_selected_path);
		std::cout << std::endl;
	};

	{
		if (destination_dir == source_dir ||
			destination_dir.lexically_relative(source_dir).c_str()[0] != L'.' ||
			source_dir.lexically_relative(destination_dir).c_str()[0] != L'.') { // !(../**)
			std::cout << "Error: The source directory and destination directory overlap." << std::endl;
			return;
		};

		if (std::filesystem::exists(destination_dir / source_dir.filename())) {
			std::cout << "Error: The directory (" << wstr2str(destination_dir) << ")" <<
			" already contains something named (" << wstr2str(source_dir.filename()) << ")." << std::endl;
			return;
		};

		std::list<std::filesystem::path> dirs = {source_dir};

		const auto time_start = std::chrono::system_clock::now();
		for (const std::filesystem::path& dir : dirs) {
			const std::filesystem::path subdir_path = destination_dir / source_dir.filename() / dir.lexically_relative(source_dir);
			std::cout << "The directory will be created: " << subdir_path << std::endl;

			std::filesystem::create_directory(subdir_path);

			for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator{dir}) {
				if (entry.is_directory()) {
					dirs.push_back(entry.path());
				} else if (entry.is_regular_file()) {
					const std::filesystem::path destination_file_path = subdir_path / entry.path().filename();
					std::cout << "A hard link will be created: (" << wstr2str(entry.path()) << ") -> (" << wstr2str(destination_file_path) << ")" << std::endl;
					std::filesystem::create_hard_link(entry.path(), destination_file_path);
				} else {
					std::cout << R"(Warning: It's not a directory and it's not a "regular file": )" << wstr2str(entry.path()) << std::endl;
				};
			};
		};
		const auto time_end = std::chrono::system_clock::now();

		std::cout << std::endl;

		std::cout << "Done! (" << std::chrono::duration<double, std::ratio<1, 1>>(time_end - time_start) << ")" << std::endl;
	};
};

int main() {
	SetConsoleOutputCP(CP_UTF8);
	select_and_copy();
	std::cin.get();
	return 0;
};