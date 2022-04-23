#ifndef __VEHICLE_TRACKING_TYPES__
#define __VEHICLE_TRACKING_TYPES__

#include <glib.h>
#include <gst/gst.h>
#include <cstdint>
#include <string>
#include <map>
#include <array>
#include <unordered_map>
#include <memory>
#include "kafkaproducer.h"

namespace {
constexpr auto N = 5;
} // namespace


namespace kafkaproducer {

struct KafkaInfo {
  KafkaInfo() = delete;
  explicit KafkaInfo(const std::string &endpoint, const std::string &topic):
    mEndpoint{endpoint},
    mTopic{topic} {}
  KafkaInfo(const KafkaInfo &) = default;
  KafkaInfo(KafkaInfo &&) = default;
  ~KafkaInfo() = default;
  std::string mEndpoint;
  std::string mTopic;
};
using kafka_info_t = struct KafkaInfo;
} // namespace kafkaproducer

namespace vehicletracking {

using arg_count_t = int;
using arg_var_t = char **;
using bus_id_t = guint;
using loop_t = GMainLoop *;
using pipeline_t = GstElement *;

using buscb_t = gboolean(*)(GstBus *, GstMessage *, gpointer);

using producer_t = std::shared_ptr<::kafkaproducer::KafkaProducer>;

} // namespace vehicletracking

namespace metadata {

struct DisplayInfo {
  std::string fps{"FPS Info: "};
  std::string roi;
  std::map<std::string, std::uint32_t> crossings;
};
using display_info_t = struct DisplayInfo;

using crossing_t = std::array<std::uint16_t, N>;
using crossings_t = std::array<crossing_t, N>;

using object_entry_t = std::unordered_map<std::uint64_t, std::size_t>;

using meta_producer_t = std::weak_ptr<::kafkaproducer::KafkaProducer>;

} // namespace metadata

#endif


