#pragma once

#include <crab/crab.hpp>
#include <filesystem>
#include <fstream>
#include "log.hpp"
#include "util.hpp"

namespace schwifty::krabby {

using namespace schwifty::logger;
namespace http = crab::http;

class mountpoint {
public:
	mountpoint(std::string point, std::filesystem::path path) : point_{point}, path_{path} {
		log::info("creating mountpoint '{}' -> '{}'", point, path_.string());
	}

	bool handle(http::Client *who, http::Request &request) {
		log::trace("checking point '{}' vs path '{}'", point_, request.header.path.substr(0, point_.size()));
		if (request.header.path.substr(0, point_.size()) == point_) {
			auto path = request.header.path.substr(point_.size());
			path      = remove_leading_slash(path);

			auto ext       = std::string{"txt"};  // assume txt by default
			auto ext_start = path.find_last_of('.');
			if (ext_start != std::string::npos) {
				ext = str_tolower(path.substr(ext_start + 1));
			}

			auto [mime, mime_params] = mime_type_for(ext);
			log::info("mime-type detected as '{}'", mime);

			auto p = (path_ / path).string();
			log::info("handling path: '{}' -> '{}'", path, p);

			try {
				auto bytes = read_file(p);

				http::ResponseHeader r;
				r.status = 200;
				r.set_content_type(mime, mime_params);
				r.content_length = bytes.size();

				who->write(std::move(r), crab::BUFFER_ONLY);
				who->write(bytes.data(), bytes.size());

				log::info("file handled from path '{}'", p);
			} catch (std::runtime_error &err) {
				log::warn("could not get file from path '{}'", p);
				who->write(http::Response::simple_html(404));
			}

			return true;
		}

		return false;
	}

private:
	std::tuple<std::string, std::string> mime_type_for(std::string_view ext) {
		if (ext == "html" || ext == "htm") {
			return {"text/html", "charset=utf-8"};
		}
		if (ext == "js" || ext == "mjs") {
			return {"text/javascript", ""};
		}
		if (ext == "css") {
			return {"text/css", ""};
		}
		if (ext == "png") {
			return {"image/png", ""};
		}
		if (ext == "jpg" || ext == "jpeg") {
			return {"image/jpeg", ""};
		}
		if (ext == "gif") {
			return {"image/gif", ""};
		}

		return {"text/plain", "charset=utf-8"};
	}

	// todo: rewrite to use a file wrapper once hrissan makes one for crablib
	std::vector<uint8_t> read_file(std::filesystem::path filepath) {
		std::ifstream ifs(filepath, std::ios::binary | std::ios::ate);

		if (!ifs) {
			throw std::runtime_error(filepath.string() + ": " + std::strerror(errno));
		}

		auto end = ifs.tellg();
		ifs.seekg(0, std::ios::beg);

		auto size = std::size_t(end - ifs.tellg());

		if (size == 0)  // avoid undefined behavior
		{
			return {};
		}

		std::vector<uint8_t> buffer(size);
		if (!ifs.read((char *)buffer.data(), buffer.size())) {
			throw std::runtime_error(filepath.string() + ": " + std::strerror(errno));
		}

		return buffer;
	}

	std::string point_;
	std::filesystem::path path_;
};

}  // namespace schwifty::krabby