
#include <iostream>
#include <gst/gst.h>
#include <glib.h>

#include <librdkafka/rdkafkacpp.h>

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
      std::cerr << "ERROR from element " << GST_OBJECT_NAME (msg->src) << " : " << error->message << std::endl;
      if (debug) {
        std::cerr << "Error details: " << debug << std::endl;
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

void kafka_call(RdKafka::Event &event) {
  switch (event.type()) {
    case RdKafka::Event::EVENT_ERROR:
    {
      std::cerr << "ERROR (" << RdKafka::err2str(event.err())
                << "): " << event.str() << std::endl;
      break;
    }
    case RdKafka::Event::EVENT_STATS:
    {
      std::cout << "\"STATS\": " << event.str() << std::endl;
      break;
    }
    case RdKafka::Event::EVENT_LOG:
    {
      std::cout << "\"LOG\": " << event.str() << std::endl;
      break;
    }
    default:
    {
      std::cout << "EVENT " << event.type() << " ("
                << RdKafka::err2str(event.err()) << "): " << event.str()
                << std::endl;
      break;
    }
  }
}

constexpr auto KAFKA_ENDPOINT = "192.168.50.10:9092";
constexpr auto KAFKA_TOPIC = "vehicletraffic";

} // namespace

int main(int argc, char *argv[])
{
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <elementary H264 filename> <mp4 output filename>" << std::endl;
    return -1;
  }

  vehicletracking::VehicleTrackingPipeline vtp{argc,
    argv, kafkaproducer::KafkaInfo{KAFKA_ENDPOINT, KAFKA_TOPIC}};
  auto ret = vtp.initialize(bus_call, kafka_call);
  if (vehicletracking::ERR_SUCCESS != ret) {
    std::cerr << "Unable to initialize vehicle tracking pipeline. Returned error code: " << ret << std::endl;
    return ret;
  }
  vtp.run();
  vtp.printCrossings();

  return 0;
}