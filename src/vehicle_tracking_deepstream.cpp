
#include <gst/gst.h>
#include <glib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <map>
#include <sstream>
#include <cuda_runtime_api.h>
#include "gstnvdsmeta.h"
#include "nvds_analytics_meta.h"
#include "nvdsmeta.h"

#define PGIE_CONFIG_FILE  "cfg/pgie_config.txt"
#define MAX_DISPLAY_LEN 64

#define TRACKER_CONFIG_FILE "cfg/tracker_config.txt"
#define MAX_TRACKING_ID_LEN 16

#define PGIE_CLASS_ID_BUS 0
#define PGIE_CLASS_ID_CAR 1

/* The muxer output resolution must be set if the input streams will be of
 * different resolution. The muxer will scale all the input frames to this
 * resolution. */
#define MUXER_OUTPUT_WIDTH 1920
#define MUXER_OUTPUT_HEIGHT 1080

/* Muxer batch formation timeout, for e.g. 40 millisec. Should ideally be set
 * based on the fastest source's framerate. */
#define MUXER_BATCH_TIMEOUT_USEC 40000

gint frame_number = 0;


typedef struct DisplayInfoTag {
  std::string roi;
  std::map<std::string, uint32_t> crossings;
} DisplayInfo;


void set_text(NvOSD_TextParams *txt_params, const int xOffset, const int yOffset, const std::string &display_text) {
    txt_params->display_text = (char*)g_malloc0 (MAX_DISPLAY_LEN);
    snprintf(txt_params->display_text, MAX_DISPLAY_LEN, "%s", display_text.c_str());

    // Now set the offsets where the string should appear
    txt_params->x_offset = xOffset;
    txt_params->y_offset = yOffset;

    // Font , font-color and font-size
    txt_params->font_params.font_name = "Serif";
    txt_params->font_params.font_size = 10;
    txt_params->font_params.font_color.red = 1.0;
    txt_params->font_params.font_color.green = 1.0;
    txt_params->font_params.font_color.blue = 1.0;
    txt_params->font_params.font_color.alpha = 1.0;

    // Text background color
    txt_params->set_bg_clr = 1;
    txt_params->text_bg_clr.red = 0.0;
    txt_params->text_bg_clr.green = 0.0;
    txt_params->text_bg_clr.blue = 0.0;
    txt_params->text_bg_clr.alpha = 1.0; 
}

void display_info_to_frame(NvDsBatchMeta *batch_meta, NvDsFrameMeta * const frame_meta, const DisplayInfo &displayInfo) {
    NvDsDisplayMeta *display_meta = NULL;
    int elementsInDisplay = 0;
    int xOffset = 10;
    int yOffset = 12;
    display_meta = nvds_acquire_display_meta_from_pool(batch_meta);
    display_meta->num_labels = 1 + displayInfo.crossings.size();

    NvOSD_TextParams *txt_params  = &display_meta->text_params[elementsInDisplay++];

    set_text(txt_params, xOffset, yOffset, displayInfo.roi);

    int offsetIdx = 0;
    std::stringstream out;
    for (const auto &crossing: displayInfo.crossings) {
      if (elementsInDisplay >= MAX_ELEMENTS_IN_DISPLAY_META - 1) {
        nvds_add_display_meta_to_frame(frame_meta, display_meta);
        return;
      }
      
      out << crossing.first.c_str() << " = " << crossing.second;
      
      NvOSD_TextParams *txt_params  = &display_meta->text_params[elementsInDisplay++];
      if (offsetIdx++ % 2 == 0) {
        yOffset += 30;
        xOffset = 10;
      } else {
        xOffset += MAX_DISPLAY_LEN + 50;
      }
      
      set_text(txt_params, xOffset, yOffset, out.str());
      
      out.str(""); out.clear();
    }

    nvds_add_display_meta_to_frame(frame_meta, display_meta);
}


/* nvdsanalytics_src_pad_buffer_probe  will extract metadata received on tiler sink pad
 * and extract nvanalytics metadata etc. */
static GstPadProbeReturn
nvdsanalytics_src_pad_buffer_probe (GstPad * pad, GstPadProbeInfo * info,
    gpointer u_data)
{
    GstBuffer *buf = (GstBuffer *) info->data;
    guint num_rects = 0;
    NvDsObjectMeta *obj_meta = NULL;
    guint bus_count = 0;
    guint car_count = 0;
    NvDsMetaList * l_frame = NULL;
    NvDsMetaList * l_obj = NULL;

    NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta (buf);

    for (l_frame = batch_meta->frame_meta_list; l_frame != NULL;
      l_frame = l_frame->next) {
        NvDsFrameMeta *frame_meta = (NvDsFrameMeta *) (l_frame->data);
        std::stringstream out_string;
        bus_count = 0;
        num_rects = 0;
        car_count = 0;
        for (l_obj = frame_meta->obj_meta_list; l_obj != NULL;
                l_obj = l_obj->next) {
            obj_meta = (NvDsObjectMeta *) (l_obj->data);
            if (obj_meta->class_id == PGIE_CLASS_ID_BUS) {
                bus_count++;
                num_rects++;
            }
            if (obj_meta->class_id == PGIE_CLASS_ID_CAR) {
                car_count++;
                num_rects++;
            }

            // Access attached user meta for each object
            for (NvDsMetaList *l_user_meta = obj_meta->obj_user_meta_list; l_user_meta != NULL;
                    l_user_meta = l_user_meta->next) {
                NvDsUserMeta *user_meta = (NvDsUserMeta *) (l_user_meta->data);
                if(user_meta->base_meta.meta_type == NVDS_USER_OBJ_META_NVDSANALYTICS)
                {
                    NvDsAnalyticsObjInfo * user_meta_data = (NvDsAnalyticsObjInfo *)user_meta->user_meta_data;
                    if (user_meta_data->dirStatus.length()){
                        g_print ("object %lu moving in %s\n", obj_meta->object_id, user_meta_data->dirStatus.c_str());
                    }
                }
            }
        }
        DisplayInfo displayInfo;
        /* Iterate user metadata in frames to search analytics metadata */
        for (NvDsMetaList * l_user = frame_meta->frame_user_meta_list;
                l_user != NULL; l_user = l_user->next) {
            NvDsUserMeta *user_meta = (NvDsUserMeta *) l_user->data;
            if (user_meta->base_meta.meta_type != NVDS_USER_FRAME_META_NVDSANALYTICS)
                continue;
            /* convert to  metadata */
            NvDsAnalyticsFrameMeta *meta =
                (NvDsAnalyticsFrameMeta *) user_meta->user_meta_data;
            /* Get the labels from nvdsanalytics config file */
            for (std::pair<std::string, uint32_t> status : meta->objInROIcnt){
                out_string << "Vehicles in ";
                out_string << status.first;
                out_string << " = ";
                out_string << status.second;
                displayInfo.roi = out_string.str();
            }
            for (std::pair<std::string, uint32_t> status : meta->objLCCumCnt){
                out_string << " LineCrossing Cumulative ";
                out_string << status.first;
                out_string << " = ";
                out_string << status.second;
                displayInfo.crossings.insert(status);
            }
            for (std::pair<std::string, uint32_t> status : meta->objLCCurrCnt){

                out_string << " LineCrossing Current Frame ";
                out_string << status.first;
                out_string << " = ";
                out_string << status.second;
            }
            for (std::pair<std::string, bool> status : meta->ocStatus){
                out_string << " Overcrowding status ";
                out_string << status.first;
                out_string << " = ";
                out_string << status.second;
            }
        }
        
        display_info_to_frame(batch_meta, frame_meta, displayInfo);
        
        g_print ("Frame Number = %d of Stream = %d, Number of objects = %d "
                "Bus Count = %d Car Count = %d %s\n",
            frame_meta->frame_num, frame_meta->pad_index,
            num_rects, bus_count, car_count, out_string.str().c_str());
    }
    return GST_PAD_PROBE_OK;
}


/* This is the buffer probe function that we have registered on the sink pad
 * of the OSD element. All the infer elements in the pipeline shall attach
 * their metadata to the GstBuffer, here we will iterate & process the metadata
 * forex: class ids to strings, counting of class_id objects etc. */
/*static GstPadProbeReturn
osd_sink_pad_buffer_probe (GstPad * pad, GstPadProbeInfo * info,
    gpointer u_data)
{
    GstBuffer *buf = (GstBuffer *) info->data;
    guint num_rects = 0;
    NvDsObjectMeta *obj_meta = NULL;
    guint bus_count = 0;
    guint car_count = 0;
    NvDsMetaList * l_frame = NULL;
    NvDsMetaList * l_obj = NULL;
    NvDsDisplayMeta *display_meta = NULL;

    NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta (buf);

    for (l_frame = batch_meta->frame_meta_list; l_frame != NULL;
      l_frame = l_frame->next) {
        NvDsFrameMeta *frame_meta = (NvDsFrameMeta *) (l_frame->data);
        int offset = 0;
        for (l_obj = frame_meta->obj_meta_list; l_obj != NULL;
                l_obj = l_obj->next) {
            obj_meta = (NvDsObjectMeta *) (l_obj->data);
            if (obj_meta->class_id == PGIE_CLASS_ID_BUS) {
                bus_count++;
                num_rects++;
            }
            if (obj_meta->class_id == PGIE_CLASS_ID_CAR) {
                car_count++;
                num_rects++;
            }
        }
        display_meta = nvds_acquire_display_meta_from_pool(batch_meta);
        NvOSD_TextParams *txt_params  = &display_meta->text_params[0];
        display_meta->num_labels = 1;
        txt_params->display_text = g_malloc0 (MAX_DISPLAY_LEN);
        offset = snprintf(txt_params->display_text, MAX_DISPLAY_LEN, "Bus = %d ", bus_count);
        offset = snprintf(txt_params->display_text + offset , MAX_DISPLAY_LEN, "Car = %d ", car_count);

        // Now set the offsets where the string should appear
        txt_params->x_offset = 10;
        txt_params->y_offset = 12;

        // Font , font-color and font-size
        txt_params->font_params.font_name = "Serif";
        txt_params->font_params.font_size = 10;
        txt_params->font_params.font_color.red = 1.0;
        txt_params->font_params.font_color.green = 1.0;
        txt_params->font_params.font_color.blue = 1.0;
        txt_params->font_params.font_color.alpha = 1.0;

        // Text background color
        txt_params->set_bg_clr = 1;
        txt_params->text_bg_clr.red = 0.0;
        txt_params->text_bg_clr.green = 0.0;
        txt_params->text_bg_clr.blue = 0.0;
        txt_params->text_bg_clr.alpha = 1.0;

        nvds_add_display_meta_to_frame(frame_meta, display_meta);
    }

    g_print ("Frame Number = %d Number of objects = %d "
            "Bus Count = %d Car Count = %d\n",
            frame_number, num_rects, bus_count, car_count);
    frame_number++;
    return GST_PAD_PROBE_OK;
}
*/
static gboolean
bus_call (GstBus * bus, GstMessage * msg, gpointer data)
{
  GMainLoop *loop = (GMainLoop *) data;
  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
      g_print ("End of stream\n");
      g_main_loop_quit (loop);
      break;
    case GST_MESSAGE_ERROR:{
      gchar *debug;
      GError *error;
      gst_message_parse_error (msg, &error, &debug);
      g_printerr ("ERROR from element %s: %s\n",
          GST_OBJECT_NAME (msg->src), error->message);
      if (debug)
        g_printerr ("Error details: %s\n", debug);
      g_free (debug);
      g_error_free (error);
      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }
  return TRUE;
}

/* Tracker config parsing */

#define CHECK_ERROR(error) \
    if (error) { \
        g_printerr ("Error while parsing config file: %s\n", error->message); \
        goto done; \
    }

#define CONFIG_GROUP_TRACKER "tracker"
#define CONFIG_GROUP_TRACKER_WIDTH "tracker-width"
#define CONFIG_GROUP_TRACKER_HEIGHT "tracker-height"
#define CONFIG_GROUP_TRACKER_LL_CONFIG_FILE "ll-config-file"
#define CONFIG_GROUP_TRACKER_LL_LIB_FILE "ll-lib-file"
#define CONFIG_GROUP_TRACKER_ENABLE_BATCH_PROCESS "enable-batch-process"
#define CONFIG_GPU_ID "gpu-id"

static gchar *
get_absolute_file_path (gchar *cfg_file_path, gchar *file_path)
{
  gchar abs_cfg_path[PATH_MAX + 1];
  gchar *abs_file_path;
  gchar *delim;

  if (file_path && file_path[0] == '/') {
    return file_path;
  }

  if (!realpath (cfg_file_path, abs_cfg_path)) {
    g_free (file_path);
    return NULL;
  }

  // Return absolute path of config file if file_path is NULL.
  if (!file_path) {
    abs_file_path = g_strdup (abs_cfg_path);
    return abs_file_path;
  }

  delim = g_strrstr (abs_cfg_path, "/");
  *(delim + 1) = '\0';

  abs_file_path = g_strconcat (abs_cfg_path, file_path, NULL);
  g_free (file_path);

  return abs_file_path;
}

static gboolean
set_tracker_properties (GstElement *nvtracker)
{
  gboolean ret = FALSE;
  GError *error = NULL;
  gchar **keys = NULL;
  gchar **key = NULL;
  GKeyFile *key_file = g_key_file_new ();

  if (!g_key_file_load_from_file (key_file, TRACKER_CONFIG_FILE, G_KEY_FILE_NONE,
          &error)) {
    g_printerr ("Failed to load config file: %s\n", error->message);
    return FALSE;
  }

  keys = g_key_file_get_keys (key_file, CONFIG_GROUP_TRACKER, NULL, &error);
  CHECK_ERROR (error);

  for (key = keys; *key; key++) {
    if (!g_strcmp0 (*key, CONFIG_GROUP_TRACKER_WIDTH)) {
      gint width =
          g_key_file_get_integer (key_file, CONFIG_GROUP_TRACKER,
          CONFIG_GROUP_TRACKER_WIDTH, &error);
      CHECK_ERROR (error);
      g_object_set (G_OBJECT (nvtracker), "tracker-width", width, NULL);
    } else if (!g_strcmp0 (*key, CONFIG_GROUP_TRACKER_HEIGHT)) {
      gint height =
          g_key_file_get_integer (key_file, CONFIG_GROUP_TRACKER,
          CONFIG_GROUP_TRACKER_HEIGHT, &error);
      CHECK_ERROR (error);
      g_object_set (G_OBJECT (nvtracker), "tracker-height", height, NULL);
    } else if (!g_strcmp0 (*key, CONFIG_GPU_ID)) {
      guint gpu_id =
          g_key_file_get_integer (key_file, CONFIG_GROUP_TRACKER,
          CONFIG_GPU_ID, &error);
      CHECK_ERROR (error);
      g_object_set (G_OBJECT (nvtracker), "gpu_id", gpu_id, NULL);
    } else if (!g_strcmp0 (*key, CONFIG_GROUP_TRACKER_LL_CONFIG_FILE)) {
      char* ll_config_file = get_absolute_file_path (TRACKER_CONFIG_FILE,
                g_key_file_get_string (key_file,
                    CONFIG_GROUP_TRACKER,
                    CONFIG_GROUP_TRACKER_LL_CONFIG_FILE, &error));
      CHECK_ERROR (error);
      g_object_set (G_OBJECT (nvtracker), "ll-config-file", ll_config_file, NULL);
    } else if (!g_strcmp0 (*key, CONFIG_GROUP_TRACKER_LL_LIB_FILE)) {
      char* ll_lib_file = get_absolute_file_path (TRACKER_CONFIG_FILE,
                g_key_file_get_string (key_file,
                    CONFIG_GROUP_TRACKER,
                    CONFIG_GROUP_TRACKER_LL_LIB_FILE, &error));
      CHECK_ERROR (error);
      g_object_set (G_OBJECT (nvtracker), "ll-lib-file", ll_lib_file, NULL);
    } else if (!g_strcmp0 (*key, CONFIG_GROUP_TRACKER_ENABLE_BATCH_PROCESS)) {
      gboolean enable_batch_process =
          g_key_file_get_integer (key_file, CONFIG_GROUP_TRACKER,
          CONFIG_GROUP_TRACKER_ENABLE_BATCH_PROCESS, &error);
      CHECK_ERROR (error);
      g_object_set (G_OBJECT (nvtracker), "enable_batch_process",
                    enable_batch_process, NULL);
    } else {
      g_printerr ("Unknown key '%s' for group [%s]", *key,
          CONFIG_GROUP_TRACKER);
    }
  }

  ret = TRUE;
done:
  if (error) {
    g_error_free (error);
  }
  if (keys) {
    g_strfreev (keys);
  }
  if (!ret) {
    g_printerr ("%s failed", __func__);
  }
  return ret;
}

int
main (int argc, char *argv[])
{
  GMainLoop *loop = NULL;
  GstElement *pipeline = NULL, *source = NULL, *h264parser = NULL,
      *decoder = NULL, *streammux = NULL, *sink = NULL, *pgie = NULL, *nvvidconv = NULL,
      *nvosd = NULL, *nvvidconv_postosd = NULL, *cap_filter = NULL, *encoder = NULL,  *nvtracker = NULL,
      *codecparse = NULL, *mux = NULL, *nvdsanalytics = NULL;
  g_print ("With tracker\n");
  GstElement *transform = NULL;
  GstBus *bus = NULL;
  guint bus_watch_id = 0;
  //GstPad *osd_sink_pad = NULL;
  GstPad *nvdsanalytics_src_pad = NULL;

  int current_device = -1;
  cudaGetDevice(&current_device);
  struct cudaDeviceProp prop;
  cudaGetDeviceProperties(&prop, current_device);

  /* Check input arguments */
  if (argc != 2) {
    g_printerr ("Usage: %s <elementary H264 filename>\n", argv[0]);
    return -1;
  }

  /* Standard GStreamer initialization */
  gst_init (&argc, &argv);
  loop = g_main_loop_new (NULL, FALSE);

  /* Create gstreamer elements */

  /* Create Pipeline element that will be a container of other elements */
  pipeline = gst_pipeline_new ("dstest2-pipeline");

  /* Source element for reading from the file */
  source = gst_element_factory_make ("filesrc", "file-source");

  /* Since the data format in the input file is elementary h264 stream,
   * we need a h264parser */
  h264parser = gst_element_factory_make ("h264parse", "h264-parser");

  /* Use nvdec_h264 for hardware accelerated decode on GPU */
  decoder = gst_element_factory_make ("nvv4l2decoder", "nvv4l2-decoder");

  /* Create nvstreammux instance to form batches from one or more sources. */
  streammux = gst_element_factory_make ("nvstreammux", "stream-muxer");

  if (!pipeline || !streammux) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  GstElement *queue1 = NULL, *queue2 = NULL, 
             *queue3 = NULL, *queue4 = NULL,
             *queue5 = NULL, *queue6 = NULL;  
  queue1 = gst_element_factory_make ("queue", "queue1");
  queue2 = gst_element_factory_make ("queue", "queue2");
  queue3 = gst_element_factory_make ("queue", "queue3");
  queue4 = gst_element_factory_make ("queue", "queue4");
  queue5 = gst_element_factory_make ("queue", "queue5");
  queue6 = gst_element_factory_make ("queue", "queue6");

  if (!queue1 || !queue2 || !queue3 || !queue4 || !queue5 || !queue6) {
    g_printerr("Failed to create queues");
    return -1;
  }

  /* Use nvinfer to run inferencing on decoder's output,
   * behaviour of inferencing is set through config file */
  pgie = gst_element_factory_make ("nvinfer", "primary-nvinference-engine");

  /* We need to have a tracker to track the identified objects */
  nvtracker = gst_element_factory_make ("nvtracker", "tracker");

  /* Use nvdsanalytics to perform analytics on object */
  nvdsanalytics = gst_element_factory_make ("nvdsanalytics", "nvdsanalytics");

  /* Use convertor to convert from NV12 to RGBA as required by nvosd */
  nvvidconv = gst_element_factory_make ("nvvideoconvert", "nvvideo-converter");

  /* Create OSD to draw on the converted RGBA buffer */
  nvosd = gst_element_factory_make ("nvdsosd", "nv-onscreendisplay");

  nvvidconv_postosd = gst_element_factory_make ("nvvideoconvert", "nvvideo-converter-postosd");

  cap_filter = gst_element_factory_make ("capsfilter", "filter");
  if (!cap_filter) {
     g_printerr ("Failed to create capsfilter. Exiting.\n");
    return -1;
  }
  GstCaps *caps = NULL;
  caps = gst_caps_from_string ("video/x-raw(memory:NVMM), format=I420");
  g_object_set(G_OBJECT (cap_filter), "caps", caps, NULL);

  encoder = gst_element_factory_make ("nvv4l2h264enc", "encoder");
  if (!encoder) {
    g_printerr ("Failed to create encoder. Exiting.\n");
    return -1;
  }
  g_object_set(G_OBJECT (encoder), "preset-level", 1, NULL);
  g_object_set(G_OBJECT (encoder), "insert-sps-pps", 1, NULL);
  g_object_set(G_OBJECT (encoder), "bufapi-version", 1, NULL);
  g_object_set(G_OBJECT (encoder), "bitrate", 4000000, NULL);

  codecparse = gst_element_factory_make ("h264parse", "h264-parser-codec");
  if (!codecparse) {
    g_printerr ("Failed to create codecparse. Exiting.\n");
    return -1;
  }

  mux = gst_element_factory_make("mp4mux", "mux");
  if (!mux) {
    g_printerr ("Failed to create mux. Exiting.\n");
    return -1;
  }

  /* Finally render the osd output */
  if(prop.integrated) {
    transform = gst_element_factory_make ("nvegltransform", "nvegl-transform");
  }
  //sink = gst_element_factory_make ("nveglglessink", "nvvideo-renderer");
  sink = gst_element_factory_make ("filesink", "filesink");
  if (!sink) {
    g_printerr ("Failed to create sink. Exiting.\n");
    return -1;
  }
  g_object_set(G_OBJECT (sink), "location", "/tmp/out-analytics.mp4", NULL);


  if (!source || !h264parser || !decoder || !pgie ||
      !nvtracker || !nvvidconv || !nvosd || !nvvidconv_postosd || !nvdsanalytics) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  if(!transform && prop.integrated) {
      g_printerr ("One tegra element could not be created. Exiting.\n");
      return -1;
  }

  /* Set the input filename to the source element */
  g_object_set (G_OBJECT (source), "location", argv[1], NULL);

  g_object_set (G_OBJECT (streammux), "batch-size", 1, NULL);

  g_object_set (G_OBJECT (streammux), "width", MUXER_OUTPUT_WIDTH, "height",
      MUXER_OUTPUT_HEIGHT,
      "batched-push-timeout", MUXER_BATCH_TIMEOUT_USEC, NULL);

  /* Set all the necessary properties of the nvinfer element,
   * the necessary ones are : */
  g_object_set (G_OBJECT (pgie), "config-file-path", PGIE_CONFIG_FILE, NULL);

  /* Set necessary properties of the tracker element. */
  if (!set_tracker_properties(nvtracker)) {
    g_printerr ("Failed to set tracker properties. Exiting.\n");
    return -1;
  }

  /* Configure the nvdsanalytics element for using the particular analytics config file*/
  g_object_set (G_OBJECT (nvdsanalytics),
      "config-file", "cfg/config_nvdsanalytics.txt",
       NULL);

  /* we add a message handler */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

  /* Set up the pipeline */
  /* we add all elements into the pipeline */
  /* decoder | pgie1 | nvtracker | sgie1 | sgie2 | sgie3 | etc.. */

  /*if(prop.integrated) {
    gst_bin_add_many (GST_BIN (pipeline),
        source, h264parser, decoder, streammux, pgie, nvtracker, //sgie1, sgie2, sgie3,
        nvvidconv, nvosd, transform, sink, NULL);
  }
  else {
    gst_bin_add_many (GST_BIN (pipeline),
        source, h264parser, decoder, streammux, pgie, nvtracker, //sgie1, sgie2, sgie3,
        nvvidconv, nvosd, sink, NULL);*/

    gst_bin_add_many (GST_BIN (pipeline),
        source, h264parser, decoder, streammux, queue1, pgie, queue2, nvtracker, queue3, nvdsanalytics, queue4,
        nvvidconv, queue5, nvosd, queue6, nvvidconv_postosd, cap_filter, encoder, codecparse, mux, sink, NULL);
  //}

  GstPad *sinkpad, *srcpad;
  gchar pad_name_sink[16] = "sink_0";
  gchar pad_name_src[16] = "src";

  sinkpad = gst_element_get_request_pad (streammux, pad_name_sink);
  if (!sinkpad) {
    g_printerr ("Streammux request sink pad failed. Exiting.\n");
    return -1;
  }

  srcpad = gst_element_get_static_pad (decoder, pad_name_src);
  if (!srcpad) {
    g_printerr ("Decoder request src pad failed. Exiting.\n");
    return -1;
  }

  if (gst_pad_link (srcpad, sinkpad) != GST_PAD_LINK_OK) {
      g_printerr ("Failed to link decoder to stream muxer. Exiting.\n");
      return -1;
  }

  gst_object_unref (sinkpad);
  gst_object_unref (srcpad);

  /* Link the elements together */
  if (!gst_element_link_many (source, h264parser, decoder, NULL)) {
    g_printerr ("Elements could not be linked: 1. Exiting.\n");
    return -1;
  }

  /*if(prop.integrated) {
    if (!gst_element_link_many (streammux, pgie, nvtracker, nvvidconv, nvosd, transform, sink, NULL)) {
      g_printerr ("Elements could not be linked. Exiting.\n");
      return -1;
    }
  }
  else {*/
    //if (!gst_element_link_many (streammux, pgie, nvtracker, nvvidconv, nvosd, sink, NULL)) {
    if (!gst_element_link_many (streammux, queue1, pgie, queue2, nvtracker, queue3, nvdsanalytics, queue4,
       nvvidconv, queue5, nvosd, queue6, nvvidconv_postosd, cap_filter, encoder, codecparse, mux, sink, NULL)) {
      g_printerr ("Elements could not be linked. Exiting.\n");
      return -1;
    }
  //}

  /* Lets add probe to get informed of the meta data generated, we add probe to
   * the sink pad of the osd element, since by that time, the buffer would have
   * had got all the metadata. */
  /*osd_sink_pad = gst_element_get_static_pad (nvosd, "sink");
  if (!osd_sink_pad)
    g_print ("Unable to get sink pad\n");
  else
    gst_pad_add_probe (osd_sink_pad, GST_PAD_PROBE_TYPE_BUFFER,
        osd_sink_pad_buffer_probe, NULL, NULL);
  gst_object_unref (osd_sink_pad);*/

  /* Lets add probe to get informed of the meta data generated, we add probe to
   * the sink pad of the nvdsanalytics element, since by that time, the buffer
   * would have had got all the metadata.
  */
  nvdsanalytics_src_pad = gst_element_get_static_pad (nvdsanalytics, "src");
  if (!nvdsanalytics_src_pad)
    g_print ("Unable to get src pad\n");
  else
    gst_pad_add_probe (nvdsanalytics_src_pad, GST_PAD_PROBE_TYPE_BUFFER,
        nvdsanalytics_src_pad_buffer_probe, NULL, NULL);
  gst_object_unref (nvdsanalytics_src_pad);

  /* Set the pipeline to "playing" state */
  g_print ("Now playing: %s\n", argv[1]);
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  /* Iterate */
  g_print ("Running...\n");
  g_main_loop_run (loop);

  /* Out of the main loop, clean up nicely */
  g_print ("Returned, stopping playback\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_print ("Deleting pipeline\n");
  gst_object_unref (GST_OBJECT (pipeline));
  g_source_remove (bus_watch_id);
  g_main_loop_unref (loop);
  return 0;
}