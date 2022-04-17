#include "vehicletrackingpipeline.h"

#include <string>
#include <array>

#include "trackerparsing.h"
#include "metadata.h"

namespace {
constexpr auto PIPELINE_NAME = "Vehicle-Tracking-Pipeline";
constexpr auto PGIE_CONFIG_FILE = "cfg/pgie_config.txt";
constexpr auto ANALYTICS_CONFIG_FILE = "cfg/config_nvdsanalytics.txt";

constexpr auto MUXER_OUTPUT_WIDTH = 1920;
constexpr auto MUXER_OUTPUT_HEIGHT = 1080;
constexpr auto MUXER_BATCH_TIMEOUT_USEC = 40000;

constexpr auto NUMBER_QUEUES = 6;

constexpr auto ELEMENT_SOURCE_FILE = "filesrc";
constexpr auto ELEMENT_PARSE_H264 = "h264parse";
constexpr auto ELEMENT_DECODER_NVV4L2 = "nvv4l2decoder";
constexpr auto ELEMENT_STREAMMUX_NV = "nvstreammux";
constexpr auto ELEMENT_INFER_NV = "nvinfer";
constexpr auto ELEMENT_TRACKER_NV = "nvtracker";
constexpr auto ELEMENT_ANALYTICS_NV = "nvdsanalytics";
constexpr auto ELEMENT_VIDEOCONVERT_NV = "nvvideoconvert";
constexpr auto ELEMENT_DSOSD_NV = "nvdsosd";
constexpr auto ELEMENT_FILTER_CAPS = "capsfilter";
constexpr auto ELEMENT_ENCODER_NVV4L2H264 = "nvv4l2h264enc";
constexpr auto ELEMENT_MUX_MP4 = "mp4mux";
constexpr auto ELEMENT_QUEUE = "queue";
constexpr auto ELEMENT_SINK_FILE = "filesink";
constexpr auto ELEMENT_SINK_FPS_DISPLAY = "fpsdisplaysink";

constexpr auto ELEMENT_NAME_SOURCE_FILE = "file-source";
constexpr auto ELEMENT_NAME_PARSE_H264 = "h264-parser";
constexpr auto ELEMENT_NAME_DECODER_NVV4L2 = "nvv4l2-decoder";
constexpr auto ELEMENT_NAME_STREAMMUX_NV = "stream-muxer";
constexpr auto ELEMENT_NAME_INFER_NV_PRIMARY = "primary-nvinference-engine";
constexpr auto ELEMENT_NAME_TRACKER_NV = "tracker";
constexpr auto ELEMENT_NAME_ANALYTICS_NV = "nvdsanalytics";
constexpr auto ELEMENT_NAME_VIDEOCONVERT_NV = "nvvideo-converter";
constexpr auto ELEMENT_NAME_DSOSD_NV = "nv-onscreendisplay";
constexpr auto ELEMENT_NAME_VIDEOCONVERT_POSTOSD_NV = "nvvideo-converter-postosd";
constexpr auto ELEMENT_NAME_FILTER_CAPS = "filter";
constexpr auto ELEMENT_NAME_ENCODER_NVV4L2H264 = "encoder";
constexpr auto ELEMENT_NAME_PARSE_H264_CODEC = "h264-parser-codec";
constexpr auto ELEMENT_NAME_MUX_MP4 = "mux";
constexpr auto ELEMENT_NAME_SINK_FILE = "filesink";
constexpr auto ELEMENT_NAME_SINK_FPS_DISPLAY = "fps-display";

constexpr auto PAD_NAME_SINK = "sink_0";
constexpr auto PAD_NAME_SRC = "src";

} // namespace

namespace vehicletracking{

VehicleTrackingPipeline::VehicleTrackingPipeline(
    const arg_count_t argc,
    arg_var_t argv)
    : mArgc{argc},
      mLoop{nullptr},
      mPipeline{nullptr},
      mBusWatchId{0},
      mCleanup{false},
      mArgv{argv} {}

VehicleTrackingPipeline::~VehicleTrackingPipeline() {
  if (!mCleanup) {
    this->cleanup();
  }
}

std::uint8_t VehicleTrackingPipeline::initialize(const buscb_t busCall) {
  gst_init (&mArgc, &mArgv);
  mLoop = g_main_loop_new (nullptr, FALSE);
  if (nullptr == mLoop) {
    return ERR_INITIALIZE_LOOP;
  }
  mPipeline = gst_pipeline_new (PIPELINE_NAME);
  if (nullptr == mPipeline) {
    return ERR_INITIALIZE_PIPELINE;
  }
  GstElement *source = nullptr;
  source = gst_element_factory_make (ELEMENT_SOURCE_FILE, ELEMENT_NAME_SOURCE_FILE);
  if (nullptr == source) {
    return ERR_INITIALIZE_SOURCE;
  }
  g_object_set (G_OBJECT (source), "location", mArgv[1], nullptr);
  GstElement *h264parser = nullptr;
  h264parser = gst_element_factory_make (ELEMENT_PARSE_H264, ELEMENT_NAME_PARSE_H264);
  if (nullptr == h264parser) {
    return ERR_INITIALIZE_H264PARSER;
  }
  GstElement *decoder = nullptr;
  decoder = gst_element_factory_make (ELEMENT_DECODER_NVV4L2, ELEMENT_NAME_DECODER_NVV4L2 );
  if (nullptr == decoder) {
    return ERR_INITIALIZE_NVV4L2DECODER;
  }
  GstElement *streammux = nullptr;
  streammux = gst_element_factory_make (ELEMENT_STREAMMUX_NV, ELEMENT_NAME_STREAMMUX_NV);
  if (nullptr == streammux) {
    return ERR_INITIALIZE_STREAMMUX;
  }
  g_object_set (G_OBJECT (streammux), "batch-size", 1, nullptr);
  g_object_set (G_OBJECT (streammux), "width", MUXER_OUTPUT_WIDTH, "height",
      MUXER_OUTPUT_HEIGHT,
      "batched-push-timeout", MUXER_BATCH_TIMEOUT_USEC, nullptr);
  GstElement *pgie = nullptr;
  pgie = gst_element_factory_make (ELEMENT_INFER_NV, ELEMENT_NAME_INFER_NV_PRIMARY);
  if (nullptr == pgie) {
    return ERR_INITIALIZE_PGIE;
  }
  g_object_set (G_OBJECT (pgie), "config-file-path", PGIE_CONFIG_FILE, nullptr);
  GstElement *nvtracker = nullptr;
  nvtracker = gst_element_factory_make (ELEMENT_TRACKER_NV, ELEMENT_NAME_TRACKER_NV);
  if (nullptr == nvtracker) {
    return ERR_INITIALIZE_NVTRACKER;
  }
  if (!::trackerparsing::setTrackerProperties(nvtracker)) {
    return ERR_SET_PROPERTIES_NVTRACKER;
  }
  GstElement *nvdsanalytics = nullptr;
  nvdsanalytics = gst_element_factory_make (ELEMENT_ANALYTICS_NV, ELEMENT_NAME_ANALYTICS_NV);
  if (nullptr == nvdsanalytics) {
    return ERR_INITIALIZE_NVANALYTICS;
  }
  g_object_set (G_OBJECT (nvdsanalytics),
    "config-file", ANALYTICS_CONFIG_FILE,
    nullptr);
  GstElement *nvvidconv = nullptr;
  nvvidconv = gst_element_factory_make (ELEMENT_VIDEOCONVERT_NV, ELEMENT_NAME_VIDEOCONVERT_NV);
  if (nullptr == nvvidconv) {
    return ERR_INITIALIZE_NVVIDEOCONVERT;
  }
  GstElement *nvosd = nullptr;
  nvosd = gst_element_factory_make (ELEMENT_DSOSD_NV, ELEMENT_NAME_DSOSD_NV);
  if (nullptr == nvosd) {
    return ERR_INITIALIZE_NVDSOSD;
  }
  GstElement *nvvidconv_postosd = nullptr;
  nvvidconv_postosd = gst_element_factory_make (ELEMENT_VIDEOCONVERT_NV, ELEMENT_NAME_VIDEOCONVERT_POSTOSD_NV);
  if (nullptr == nvvidconv_postosd) {
    return ERR_INITIALIZE_NVVIDEOCONVERT_POSTOSD;
  }
  GstElement *cap_filter = nullptr;
  cap_filter = gst_element_factory_make (ELEMENT_FILTER_CAPS, ELEMENT_NAME_FILTER_CAPS);
  if (nullptr == cap_filter) {
    return ERR_INITIALIZE_CAPSFILTER;
  }
  GstCaps *caps = nullptr;
  caps = gst_caps_from_string ("video/x-raw(memory:NVMM), format=I420");
  if (nullptr == caps) {
    return ERR_INITIALIZE_CAPSFILTER;
  }
  g_object_set(G_OBJECT (cap_filter), "caps", caps, nullptr);
  GstElement *encoder = nullptr;
  encoder = gst_element_factory_make (ELEMENT_ENCODER_NVV4L2H264, ELEMENT_NAME_ENCODER_NVV4L2H264);
  if (nullptr == encoder) {
    return ERR_INITIALIZE_ENCODER;
  }
  g_object_set(G_OBJECT (encoder), "preset-level", 1, nullptr);
  g_object_set(G_OBJECT (encoder), "insert-sps-pps", 1, nullptr);
  g_object_set(G_OBJECT (encoder), "bufapi-version", 1, nullptr);
  g_object_set(G_OBJECT (encoder), "bitrate", 4000000, nullptr);
  GstElement *codecparse = nullptr;
  codecparse = gst_element_factory_make (ELEMENT_PARSE_H264, ELEMENT_NAME_PARSE_H264_CODEC);
  if (nullptr == codecparse) {
    return ERR_INITIALIZE_CODECPARSE;
  }
  GstElement *mux = nullptr;
  mux = gst_element_factory_make(ELEMENT_MUX_MP4, ELEMENT_NAME_MUX_MP4);
  if (nullptr == mux) {
    return ERR_INITIALIZE_MUX;
  }
  GstElement *sink = nullptr;
  sink = gst_element_factory_make (ELEMENT_SINK_FILE, ELEMENT_NAME_SINK_FILE);
  if (nullptr == sink) {
    return ERR_INITIALIZE_SINK;
  }
  g_object_set(G_OBJECT (sink), "location", mArgv[2], NULL);

  GstElement *fpsSink = nullptr;
  fpsSink = gst_element_factory_make (ELEMENT_SINK_FPS_DISPLAY, ELEMENT_NAME_SINK_FPS_DISPLAY);
  if (nullptr == fpsSink) {
    return ERR_INITIALIZE_FPS_SINK;
  }
  g_object_set (G_OBJECT (fpsSink), "text-overlay", FALSE, "video-sink", sink, "sync", FALSE, NULL);
  
  std::array<GstElement*, NUMBER_QUEUES> queues{
    {gst_element_factory_make (ELEMENT_QUEUE, "queue1"),
    gst_element_factory_make (ELEMENT_QUEUE, "queue2"),
    gst_element_factory_make (ELEMENT_QUEUE, "queue3"),
    gst_element_factory_make (ELEMENT_QUEUE, "queue4"),
    gst_element_factory_make (ELEMENT_QUEUE, "queue5"),
    gst_element_factory_make (ELEMENT_QUEUE, "queue6")}};

  gst_bin_add_many (GST_BIN (mPipeline),
    source, h264parser, decoder, streammux, queues[0], pgie, queues[1], nvtracker, queues[2], nvdsanalytics, queues[3],
    nvvidconv, queues[4], nvosd, queues[5], nvvidconv_postosd, cap_filter, encoder, codecparse, mux, fpsSink, nullptr);

  this->addMessageHandler(busCall);

  GstPad *sinkPad = nullptr;
  sinkPad = gst_element_get_request_pad (streammux, PAD_NAME_SINK);
  if (nullptr == sinkPad) {
    return ERR_ADD_SINK_PAD;
  }
  GstPad *srcPad = nullptr;
  srcPad = gst_element_get_static_pad (decoder, PAD_NAME_SRC);
  if (nullptr == srcPad) {
    return ERR_ADD_SRC_PAD;
  }
  if (gst_pad_link (srcPad, sinkPad) != GST_PAD_LINK_OK) {
      return ERR_LINK_DECODER_STREAMMUXER;
  }
  gst_object_unref (sinkPad);
  gst_object_unref (srcPad);

  if (!gst_element_link_many (source, h264parser, decoder, nullptr)) {
    return ERR_LINK_SRC_PARSER_DECODER;
  }
  if (!gst_element_link_many (streammux, queues[0], pgie, queues[1], nvtracker, queues[2], nvdsanalytics, queues[3],
    nvvidconv, queues[4], nvosd, queues[5], nvvidconv_postosd, cap_filter, encoder, codecparse, mux, fpsSink, nullptr)) {
    return ERR_LINK_ALL;
  }
  
  GstPad *nvdsanalytics_src_pad = nullptr;
  nvdsanalytics_src_pad = gst_element_get_static_pad (nvdsanalytics, PAD_NAME_SRC);
  if (nullptr == nvdsanalytics_src_pad) {
    return ERR_ADD_ANALYTICS_SRC_PAD;
  }
  gst_pad_add_probe (nvdsanalytics_src_pad, GST_PAD_PROBE_TYPE_BUFFER,
    ::metadata::nvdsanalyticsSrcPadBufferProbe, (gpointer)fpsSink, NULL);
  gst_object_unref (nvdsanalytics_src_pad);

  return ERR_SUCCESS;
}

void VehicleTrackingPipeline::addMessageHandler(const buscb_t busCall) {
  GstBus *bus = nullptr;
  bus = gst_pipeline_get_bus (GST_PIPELINE (mPipeline));
  //mBusWatchId = gst_bus_add_watch (bus, busCall.target<gboolean(GstBus *, GstMessage *, gpointer)>(), mLoop);
  mBusWatchId = gst_bus_add_watch (bus, busCall, mLoop);
  gst_object_unref(bus);
}

void VehicleTrackingPipeline::run() {
  gst_element_set_state (mPipeline, GST_STATE_PLAYING);
  g_main_loop_run (mLoop);

  // Out of the main loop, clean up
  this->cleanup();
}

void VehicleTrackingPipeline::cleanup() {
  gst_element_set_state (mPipeline, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (mPipeline));
  g_source_remove (mBusWatchId);
  g_main_loop_unref (mLoop);
  mCleanup = true;
}

void VehicleTrackingPipeline::printCrossings() {
  metadata::printCrossingsMatrix();
}

} // namespace vehicletracking