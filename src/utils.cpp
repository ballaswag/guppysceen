#include "hv/requests.h"
#include "hv/hurl.h"
#include "config.h"
#include "state.h"
#include "spdlog/spdlog.h"
#include "platform.h"

#include <cmath>
#include <time.h>
#include <sstream>
#include <iomanip>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <experimental/filesystem>
#include <regex>

namespace fs = std::experimental::filesystem;

namespace KUtils {

  bool is_homed() {
    auto v = State::get_instance()
      ->get_data("/printer_state/toolhead/homed_axes"_json_pointer);
    if (!v.is_null()) {
      std::string homed_axes = v.template get<std::string>();
      return homed_axes.find("x") != std::string::npos
	      && homed_axes.find("y") != std::string::npos
	      && homed_axes.find("z") != std::string::npos;
    }
    return false;
  }

  std::string get_root_path(const std::string root_name) {
    auto roots = State::get_instance()->get_data("/roots"_json_pointer);
    json filtered;
    std::copy_if(roots.begin(), roots.end(),
		 std::back_inserter(filtered), [&root_name](const json& item) {
		   return item.contains("name") && item["name"] == root_name;
		 });

    spdlog::trace("roots {}, filtered {}", roots.dump(), filtered.dump());
    if (!filtered.empty()) {
      return filtered["/0/path"_json_pointer];
    }

    return "";
  }

  std::pair<std::string, size_t> get_thumbnail(const std::string &gcode_file, json &j, double scale) {
    auto &thumbs = j["/result/thumbnails"_json_pointer];
    if (!thumbs.is_null() && !thumbs.empty()) {
      // assume square, look for closest to 300x300
      auto scaled_width = scale * 300;
      spdlog::debug("using thumb at scaled width {}", scaled_width);
      uint32_t closest_index = 0;
      size_t thumb_width = 0;
      auto width = thumbs.at(0)["width"].is_number()
	        ? thumbs.at(0)["width"].template get<int>()
	        : std::stoi(thumbs.at(0)["width"].template get<std::string>());
      int closest = std::abs(scaled_width - width);
      for (int i = 0; i < thumbs.size(); i++) {
	      width = thumbs.at(i)["width"].is_number()
	        ? thumbs.at(i)["width"].template get<int>()
	        : std::stoi(thumbs.at(i)["width"].template get<std::string>());
	      int cur_diff = std::abs(scaled_width - width);
        if (cur_diff < closest) {
          closest = cur_diff;
          closest_index = i;
          thumb_width = width;
        }
      }

      auto &thumb = thumbs.at(closest_index);
      spdlog::debug("using thumb at index {}, {}", closest_index, thumbs.dump());

      // metadata thumbnail paths are relative to the current gcode file directory
      std::string relative_path = thumb["relative_path"].template get<std::string>();
      size_t found = gcode_file.find_last_of("/\\");
      if (found != std::string::npos) {
	      relative_path = gcode_file.substr(0, found + 1) + relative_path;
      }

      Config *conf = Config::get_instance();
      std::string moonraker_host = conf->get<std::string>("/moonraker_host");
      std::string fname = relative_path.substr(relative_path.find_last_of("/\\") + 1);
      std::string fullpath = fmt::format("{}/{}", conf->get<std::string>("/thumbnail_path"), fname);
    
      // download thumbnail
      auto gcode_root = get_root_path("gcodes");
      fullpath = fmt::format("{}/{}", gcode_root, relative_path);

      return std::make_pair(fullpath, thumb_width);
    }
    return std::make_pair("", 0);
  }

  std::string download_file(const std::string &root, const std::string &fname, const std::string &dest) {
    auto filename = fs::path(fname).filename();
    auto dest_fullpath = fs::path(dest) / filename;

    spdlog::trace("root {}, fname {}, base filename {}, dest_fp {}", root, fname,
		  filename.string(), dest_fullpath.string());
    Config *conf = Config::get_instance();
    std::string moonraker_host = conf->get<std::string>("/moonraker_host");

    std::string file_url = fmt::format("http://{}:{}/server/files/{}/{}",
					moonraker_host,
					conf->get<uint32_t>("/moonraker_port"),
					root,
					HUrl::escape(fname));
    // threadpool this
    spdlog::debug("file url {}", file_url);
    auto size = requests::downloadFile(file_url.c_str(), dest_fullpath.c_str());
    spdlog::trace("downloaded file size {}", size);

    return dest_fullpath.string();
  }

  std::vector<std::string> get_interfaces() {
    std::vector<std::string> ifaces;
    struct ifaddrs *addrs;
    getifaddrs(&addrs);
    for (struct ifaddrs *addr = addrs; addr != nullptr; addr = addr->ifa_next) {
        if (addr->ifa_addr && addr->ifa_addr->sa_family == AF_PACKET) {
	        ifaces.push_back(addr->ifa_name);
        }
    }

    freeifaddrs(addrs);
    return ifaces;
  }

  std::string interface_ip(const std::string &interface) {
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

    struct ifreq ifr{};
    strcpy(ifr.ifr_name, interface.c_str());
    ioctl(fd, SIOCGIFADDR, &ifr);
    close(fd);

    char ip[INET_ADDRSTRLEN];
    strcpy(ip, inet_ntoa(((sockaddr_in *) &ifr.ifr_addr)->sin_addr));
    return ip;
  }

  std::string get_wifi_interface() {
    std::string wpa_socket = Config::get_instance()->get<std::string>("/wpa_supplicant");
    if (fs::is_directory(fs::status(wpa_socket))) {
      for (const auto &e : fs::directory_iterator(wpa_socket)) {
        if (fs::is_socket(e.path()) && e.path().string().find("p2p") == std::string::npos) {
          return e.path().filename().string();
        }
      }
    }

    return "";
  }

  template <typename Out>
  void split(const std::string &s, char delim, Out result) {
    std::istringstream iss(s);
    std::string item;
    while (std::getline(iss, item, delim)) {
        *result++ = item;
    }
  }

  std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
  }

  std::string get_obj_name(const std::string &id) {
    size_t pos = id.find_last_of(' ');
    return id.substr(pos + 1);
  }

  std::string to_title(std::string s) {
    bool last = true;
    for (char& c : s) {
      c = last ? std::toupper(c) : std::tolower(c);
      if (c == '_') {
	c = ' ';
      }

      last = std::isspace(c);
    }
    return s;
  }


  std::string eta_string(int64_t s) {
    time_t seconds (s);
    tm p;
    gmtime_r (&seconds, &p);

    std::ostringstream os;

    if (p.tm_yday > 0)
      os << p.tm_yday << "d ";

    if (p.tm_hour > 0)
      os << p.tm_hour << "h ";

    if (p.tm_min > 0)
      os << p.tm_min << "m ";

    os << p.tm_sec << "s";
    
    return os.str();
  }

  size_t bytes_to_mb(size_t s) {
    return s / 1024 / 1024;
  }

  std::map<std::string, std::map<std::string, std::string>> parse_macros(json &m) {
    std::map<std::string, std::map<std::string, std::string>> macros;

    std::regex param_regex(R"(params\.(\w+)(.*))", std::regex_constants::icase);
    std::regex default_value_regex(R"(\|\s*default\s*\(\s*((["'])(?:\\.|[^\x02])*\2|-?[0-9][^,)]*))",
                                   std::regex_constants::icase);
    for (auto &el : m.items()) {
      std::string key = el.key();
      if (key.rfind("gcode_macro ", 0) == 0) {
        auto &gcode = el.value()["/gcode"_json_pointer];
        if (!gcode.is_null()) {
          auto macro_split = split(el.key(), ' ');
          if (macro_split.size() > 1 && macro_split[1].rfind("_", 0) != 0) {
            std::string macro_name = macro_split[1];

            const auto &gcode_str = gcode.template get<std::string>();
            auto param_begin =
                std::sregex_iterator(gcode_str.begin(), gcode_str.end(), param_regex);
            auto param_end = std::sregex_iterator();

            std::map<std::string, std::string> macro_params;
            for (std::sregex_iterator i = param_begin; i != param_end; ++i) {
              std::smatch match = *i;
              std::string param_name = match.str(1);
              std::string rest = match.str(2);
              std::smatch matches;
              std::string default_value = "";

              spdlog::trace("macro: {}, param; {}, rest: {}", macro_name, param_name, rest);

              if (std::regex_search(rest, matches, default_value_regex)) {
                default_value = matches.str(1);
              }

              macro_params.insert({param_name, default_value});
            }
            macros.insert({macro_name, macro_params});
          }
        }
      }
    }

    return macros;
  }
  }  // namespace KUtils
