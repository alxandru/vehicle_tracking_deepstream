#ifndef __VEHICLE_TRACKING_TYPES__
#define __VEHICLE_TRACKING_TYPES__

#include <glib.h>
#include <gst/gst.h>
#include <cstdint>
#include <string>
#include <map>
#include <array>
#include <unordered_map>

namespace {
constexpr auto N = 5;
} // namespace

namespace vehicletracking {

using arg_count_t = int;
using arg_var_t = char **;
using bus_id_t = guint;
using loop_t = GMainLoop *;
using pipeline_t = GstElement *;

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

} //metadata

#endif


