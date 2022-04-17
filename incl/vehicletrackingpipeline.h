#ifndef __VEHICLE_TRACKING_PIPELINE__
#define __VEHICLE_TRACKING_PIPELINE__

#include <glib.h>
#include <gst/gst.h>
#include <cstdint>
#include "types.h"

namespace vehicletracking {

constexpr auto ERR_SUCCESS = 0;
constexpr auto ERR_INITIALIZE_LOOP = 1;
constexpr auto ERR_INITIALIZE_PIPELINE = 2;
constexpr auto ERR_INITIALIZE_SOURCE = 3;
constexpr auto ERR_INITIALIZE_H264PARSER = 4;
constexpr auto ERR_INITIALIZE_NVV4L2DECODER = 5;
constexpr auto ERR_INITIALIZE_STREAMMUX = 6;
constexpr auto ERR_INITIALIZE_PGIE = 7;
constexpr auto ERR_INITIALIZE_NVTRACKER = 8;
constexpr auto ERR_INITIALIZE_NVANALYTICS = 9;
constexpr auto ERR_INITIALIZE_NVDSOSD = 10;
constexpr auto ERR_INITIALIZE_NVVIDEOCONVERT = 11;
constexpr auto ERR_INITIALIZE_NVVIDEOCONVERT_POSTOSD = 12;
constexpr auto ERR_INITIALIZE_CAPSFILTER = 13;
constexpr auto ERR_INITIALIZE_ENCODER = 14;
constexpr auto ERR_INITIALIZE_CODECPARSE = 15;
constexpr auto ERR_INITIALIZE_MUX = 16;
constexpr auto ERR_INITIALIZE_SINK = 17;
constexpr auto ERR_INITIALIZE_FPS_SINK = 18;
constexpr auto ERR_ADD_SINK_PAD = 19;
constexpr auto ERR_ADD_SRC_PAD = 20;
constexpr auto ERR_ADD_ANALYTICS_SRC_PAD = 21;
constexpr auto ERR_LINK_DECODER_STREAMMUXER = 22;
constexpr auto ERR_SET_PROPERTIES_NVTRACKER = 23;
constexpr auto ERR_LINK_SRC_PARSER_DECODER = 24;
constexpr auto ERR_LINK_ALL = 25;

class VehicleTrackingPipeline final {
 public:
  using buscb_t = gboolean(*)(GstBus *, GstMessage *, gpointer);
  VehicleTrackingPipeline() = delete;
  VehicleTrackingPipeline(const arg_count_t, arg_var_t);
  VehicleTrackingPipeline(const VehicleTrackingPipeline &) = default;
  VehicleTrackingPipeline(VehicleTrackingPipeline &&) = default;
  ~VehicleTrackingPipeline();

  std::uint8_t initialize(const buscb_t);
  void run();
  void printCrossings();
 
 private:
  void cleanup();
  void addMessageHandler(const buscb_t);
  
  arg_count_t mArgc;
  loop_t mLoop;
  pipeline_t mPipeline;
  bus_id_t mBusWatchId;
  bool mCleanup;
  arg_var_t mArgv;
};

} // namespace vehicletracking

#endif