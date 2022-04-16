
#include <iostream>
#include <gst/gst.h>
#include <glib.h>

#include "vehicletrackingpipeline.h"

namespace {

gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
  GMainLoop *loop = (GMainLoop *) data;
  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
    {
      std::cout << "End of stream" << std::endl;
      g_main_loop_quit (loop);
      break;
    }
    case GST_MESSAGE_ERROR:
    {
      gchar *debug;
      GError *error;
      gst_message_parse_error (msg, &error, &debug);
      std::cout << "ERROR from element " << GST_OBJECT_NAME (msg->src) << " : " << error->message << std::endl;
      if (debug) {
        std::cout << "Error details: " << debug << std::endl;
      }
      g_free (debug);
      g_error_free (error);
      g_main_loop_quit (loop);
      break;
    }
    default:
    {
      break;
    }
  }
  return TRUE;
}

} // namespace

int main(int argc, char *argv[])
{
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <elementary H264 filename> <mp4 output filename>" << std::endl;
    return -1;
  }

  vehicletracking::VehicleTrackingPipeline vtp{argc, argv};
  auto ret = vtp.initialize(bus_call);
  if (vehicletracking::ERR_SUCCESS != ret) {
    std::cerr << "Unable to initialize vehicle tracking pipeline. Returned error code: " << ret << std::endl;
    return ret;
  }
  vtp.run();
  vtp.printCrossings();

  return 0;
}