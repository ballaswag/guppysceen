#include "config.h"
#include "platform.h"

#include <sys/stat.h>
#include <fstream>
#include <iomanip>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

Config *Config::instance{NULL};

Config::Config() {
}

Config *Config::get_instance() {
  if (instance == NULL) {
    instance = new Config();
  }
  return instance;
}

void Config::init(std::string config_path, const std::string thumbdir) {
  path = config_path;
  data = json::parse(std::fstream(config_path));
  std::ofstream o(config_path);
  o << std::setw(2) << data << std::endl;
}

std::string Config::get_thumbnail_path() {
  return get<std::string>("/thumbnail_path");
}

std::string Config::get_wifi_interface() {
  return fs::path(get<std::string>("/wpa_supplicant"))
    .filename()
    .string();
}

std::string Config::get_path() {
    return path;
}

json &Config::get_json(const std::string &json_path) {
  return data[json::json_pointer(json_path)];
}

void Config::save() {
  std::ofstream o(path);
  o << std::setw(2) << data << std::endl;
}
