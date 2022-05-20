#include "kafkaparser.h"

#include <glib.h>
#include <iostream>

namespace {

constexpr auto KAFKA_CONFIG_FILE = "cfg/kafka_config.txt";
constexpr auto CONFIG_GROUP_KAFKA = "kafka";
constexpr auto CONFIG_GROUP_KAFKA_ENDPOINT = "endpoint";
constexpr auto CONFIG_GROUP_KAFKA_TOPIC = "topic";

#define CHECK_ERROR(error) \
  if (error) { \
    std::cerr << "Error while parsing config file: " << error->message << std::endl; \
    goto done; \
  }

} // namespace

namespace kafkaparser {

bool setKafkaProperties (kafkaproducer::kafka_info_t& kafkaInfo) {
  GError *error = nullptr;

  GKeyFile *key_file = g_key_file_new ();
  if (!g_key_file_load_from_file (key_file, KAFKA_CONFIG_FILE, G_KEY_FILE_NONE,
          &error)) {
    std::cerr << "Failed to load config file: " <<  error->message << std::endl;
    return false;
  }
  bool ret = false;
  gchar **keys = nullptr;
  keys = g_key_file_get_keys (key_file, CONFIG_GROUP_KAFKA, nullptr, &error);
  CHECK_ERROR (error);

  for(gchar** key = keys; *key != nullptr; ++key) {
    if (!g_strcmp0 (*key, CONFIG_GROUP_KAFKA_ENDPOINT)) {
      gchar* endpoint = g_key_file_get_string (key_file,
                          CONFIG_GROUP_KAFKA,
                          CONFIG_GROUP_KAFKA_ENDPOINT, &error);
      CHECK_ERROR (error);
      kafkaInfo.mEndpoint = std::string(endpoint);
    } else if (!g_strcmp0 (*key, CONFIG_GROUP_KAFKA_TOPIC)) {
      gchar* topic = g_key_file_get_string (key_file,
                    CONFIG_GROUP_KAFKA,
                    CONFIG_GROUP_KAFKA_TOPIC, &error);
      CHECK_ERROR (error);
      kafkaInfo.mTopic = std::string(topic);
    } else {
      std::cerr << "Unknown key '" << *key << "'"<< "for group [" << CONFIG_GROUP_KAFKA << "]" << std::endl;
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

} // namespace kafkaparser