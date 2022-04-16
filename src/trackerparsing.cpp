#include <glib.h>
#include <experimental/filesystem>
#include <iostream>
#include <string>

#include "trackerparsing.h"

namespace {
constexpr auto TRACKER_CONFIG_FILE = "cfg/tracker_config.txt";
constexpr auto CONFIG_GROUP_TRACKER = "tracker";
constexpr auto CONFIG_GROUP_TRACKER_WIDTH = "tracker-width";
constexpr auto CONFIG_GROUP_TRACKER_HEIGHT = "tracker-height";
constexpr auto CONFIG_GROUP_TRACKER_LL_CONFIG_FILE = "ll-config-file";
constexpr auto CONFIG_GROUP_TRACKER_LL_LIB_FILE = "ll-lib-file";
constexpr auto CONFIG_GROUP_TRACKER_ENABLE_BATCH_PROCESS = "enable-batch-process";
constexpr auto CONFIG_GPU_ID = "gpu-id";

namespace fs = std::experimental::filesystem;

#define CHECK_ERROR(error) \
  if (error) { \
    std::cerr << "Error while parsing config file: " << error->message << std::endl; \
    goto done; \
  }

std::string getAbsoluteFilePath(const char *cfgFilePath, char *filePath)
{
  if (nullptr != filePath && filePath[0] == '/') {
    return std::string(filePath);
  }
  auto cfgPath = fs::path(cfgFilePath);
  fs::path absCfgPath;
  try {
    absCfgPath = fs::canonical(cfgPath);
  } catch (const std::exception& ex) {
    std::cerr << "Canonical path for " << cfgPath << " threw exception:\n"
                  << ex.what() << '\n';
    g_free (filePath);
    return std::string();
  }
  if (nullptr == filePath) {
    return absCfgPath.string();
  }
  auto absFilePath = absCfgPath.parent_path() / filePath;
  if (!fs::exists(absFilePath)) {
    return std::string();
  }
  return absFilePath.string();
}

} // namespace

namespace trackerparsing {

bool setTrackerProperties (GstElement *nvtracker)
{
  bool ret = false;
  GError *error = nullptr;

  GKeyFile *key_file = g_key_file_new ();
  if (!g_key_file_load_from_file (key_file, TRACKER_CONFIG_FILE, G_KEY_FILE_NONE,
          &error)) {
    std::cerr << "Failed to load config file: " <<  error->message << std::endl;
    return false;
  }

  gchar **keys = nullptr;
  keys = g_key_file_get_keys (key_file, CONFIG_GROUP_TRACKER, nullptr, &error);
  CHECK_ERROR (error);

  for(gchar** key = keys; *key != nullptr; ++key) {
    if (!g_strcmp0 (*key, CONFIG_GROUP_TRACKER_WIDTH)) {
      gint width =
          g_key_file_get_integer (key_file, CONFIG_GROUP_TRACKER,
          CONFIG_GROUP_TRACKER_WIDTH, &error);
      CHECK_ERROR (error);
      g_object_set (G_OBJECT (nvtracker), CONFIG_GROUP_TRACKER_WIDTH, width, nullptr);
    } else if (!g_strcmp0 (*key, CONFIG_GROUP_TRACKER_HEIGHT)) {
      gint height =
          g_key_file_get_integer (key_file, CONFIG_GROUP_TRACKER,
          CONFIG_GROUP_TRACKER_WIDTH, &error);
      CHECK_ERROR (error);
      g_object_set (G_OBJECT (nvtracker), CONFIG_GROUP_TRACKER_WIDTH, height, nullptr);
    } else if (!g_strcmp0 (*key, CONFIG_GPU_ID)) {
      guint gpu_id =
          g_key_file_get_integer (key_file, CONFIG_GROUP_TRACKER,
          CONFIG_GPU_ID, &error);
      CHECK_ERROR (error);
      g_object_set (G_OBJECT (nvtracker), CONFIG_GPU_ID, gpu_id, nullptr);
    } else if (!g_strcmp0 (*key, CONFIG_GROUP_TRACKER_LL_CONFIG_FILE)) {
      std::string ll_config_file = ::getAbsoluteFilePath(TRACKER_CONFIG_FILE,
                g_key_file_get_string (key_file,
                    CONFIG_GROUP_TRACKER,
                    CONFIG_GROUP_TRACKER_LL_CONFIG_FILE, &error));
      CHECK_ERROR (error);
      g_object_set (G_OBJECT (nvtracker), CONFIG_GROUP_TRACKER_LL_CONFIG_FILE, ll_config_file.c_str(), nullptr);
    } else if (!g_strcmp0 (*key, CONFIG_GROUP_TRACKER_LL_LIB_FILE)) {
      std::string ll_lib_file = ::getAbsoluteFilePath(TRACKER_CONFIG_FILE,
                g_key_file_get_string (key_file,
                    CONFIG_GROUP_TRACKER,
                    CONFIG_GROUP_TRACKER_LL_LIB_FILE, &error));
      CHECK_ERROR (error);
      g_object_set (G_OBJECT (nvtracker), CONFIG_GROUP_TRACKER_LL_LIB_FILE, ll_lib_file.c_str(), nullptr);
    } else if (!g_strcmp0 (*key, CONFIG_GROUP_TRACKER_ENABLE_BATCH_PROCESS)) {
      gboolean enable_batch_process =
          g_key_file_get_integer (key_file, CONFIG_GROUP_TRACKER,
          CONFIG_GROUP_TRACKER_ENABLE_BATCH_PROCESS, &error);
      CHECK_ERROR (error);
      g_object_set (G_OBJECT (nvtracker), CONFIG_GROUP_TRACKER_ENABLE_BATCH_PROCESS,
                    enable_batch_process, nullptr);
    } else {
      std::cerr << "Unknown key '" << *key << "'"<< "for group [" << CONFIG_GROUP_TRACKER << "]" << std::endl;
    }
  }
  ret = true;
done:
  if (error != nullptr) {
    g_error_free (error);
  }
  if (keys != nullptr) {
    g_strfreev (keys);
  }
  if (!ret) {
    std::cerr << __func__ << " failed" << std::endl;
  }
  return ret;
}

} // namespace trackerparsing
