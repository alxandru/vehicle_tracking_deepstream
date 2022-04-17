#include <glib.h>
#include <iostream>
#include <vector>
#include <sstream>
#include "metadata.h"
#include "gstnvdsmeta.h"
#include "nvds_analytics_meta.h"
#include "nvdsmeta.h"
#include "types.h"

namespace {

constexpr auto MAX_DISPLAY_LEN = 70;
constexpr auto PGIE_CLASS_ID_BUS = 0;
constexpr auto PGIE_CLASS_ID_CAR = 1;
constexpr auto FONT_SERIF = "Serif";

constexpr auto CROSSING_N = "N";
constexpr auto CROSSING_NE = "NE";
constexpr auto CROSSING_SE = "SE";
constexpr auto CROSSING_SV = "SV";
constexpr auto CROSSING_NV = "NV";

constexpr auto CROSSING_IDX_N = 0;
constexpr auto CROSSING_IDX_NE = 1;
constexpr auto CROSSING_IDX_SE = 2;
constexpr auto CROSSING_IDX_SV = 3;
constexpr auto CROSSING_IDX_NV = 4;


metadata::crossings_t crossings{{
  /*N-Entry*/  {0, 0, 0, 0, 0}, // N-Exit, NE-Exit, SE-Exit, SV-Exit, NV-Exit
  /*NE-Entry*/ {0, 0, 0, 0, 0},
  /*SE-Entry*/ {0, 0, 0, 0, 0},
  /*SV-Entry*/ {0, 0, 0, 0, 0},
  /*NV-Entry*/ {0, 0, 0, 0, 0}}
};

metadata::object_entry_t objEntries;

void setText(NvOSD_TextParams *txt_params, const int xOffset, const int yOffset,
  const std::string &display_text) {
  txt_params->display_text = (char*)g_malloc0 (MAX_DISPLAY_LEN);
  snprintf(txt_params->display_text, MAX_DISPLAY_LEN, "%s", display_text.c_str());
  // Now set the offsets where the string should appear
  txt_params->x_offset = xOffset;
  txt_params->y_offset = yOffset;
  // Font , font-color and font-size
  txt_params->font_params.font_name = const_cast<char*>(FONT_SERIF);
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

void displayInfoToFrame(NvDsBatchMeta *batch_meta, NvDsFrameMeta * const frame_meta,
  const metadata::display_info_t &displayInfo) {
  NvDsDisplayMeta *display_meta = nullptr;
  int elementsInDisplay = 0;
  int xOffset = 10;
  int yOffset = 12;
  display_meta = nvds_acquire_display_meta_from_pool(batch_meta);
  display_meta->num_labels = 2 + displayInfo.crossings.size();

  NvOSD_TextParams *txt_params_fps  = &display_meta->text_params[elementsInDisplay++];
  setText(txt_params_fps, xOffset, yOffset, displayInfo.fps);

  yOffset += 30;
  xOffset = 10;
  NvOSD_TextParams *txt_params_roi  = &display_meta->text_params[elementsInDisplay++];
  setText(txt_params_roi, xOffset, yOffset, displayInfo.roi);

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
    
    setText(txt_params, xOffset, yOffset, out.str());
    
    out.str(""); out.clear();
  }

  nvds_add_display_meta_to_frame(frame_meta, display_meta);
}

std::size_t getLCIdxFromString(const std::string& crossing) {
  std::size_t pos = crossing.find("-");
  std::string prefix = crossing.substr(0, pos);
  if (0 == prefix.compare(CROSSING_N)) {
    return CROSSING_IDX_N;
  } else if (0 == prefix.compare(CROSSING_NE)) {
    return CROSSING_IDX_NE;
  } else if (0 == prefix.compare(CROSSING_SE)) {
    return CROSSING_IDX_SE;
  } else if (0 == prefix.compare(CROSSING_SV)) {
    return CROSSING_IDX_SV;
  } else if (0 == prefix.compare(CROSSING_NV)) {
    return CROSSING_IDX_NV;
  }
}

std::string getLCFromIdx(const std::size_t idx) {
  if (idx == CROSSING_IDX_N) {
    return CROSSING_N;
  } else if (idx == CROSSING_IDX_NE) {
    return CROSSING_NE;
  } else if (idx == CROSSING_IDX_SE) {
    return CROSSING_SE;
  } else if (idx == CROSSING_IDX_SV) {
    return CROSSING_SV;
  } else if (idx == CROSSING_IDX_NV) {
    return CROSSING_NV;
  }
  return "";
}

} //namespace

namespace metadata {

GstPadProbeReturn
nvdsanalyticsSrcPadBufferProbe (GstPad * pad, GstPadProbeInfo * info, gpointer u_data)
{
  GstBuffer *buf = (GstBuffer *) info->data;
  guint num_rects = 0;
  NvDsObjectMeta *obj_meta = nullptr;
  guint bus_count = 0;
  guint car_count = 0;
  NvDsMetaList * l_frame = nullptr;
  NvDsMetaList * l_obj = nullptr;

  gchar *fpsMsg = nullptr;
  g_object_get (G_OBJECT (u_data), "last-message", &fpsMsg, NULL);

  NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta (buf);

  for (l_frame = batch_meta->frame_meta_list; l_frame != nullptr;
    l_frame = l_frame->next) {
    NvDsFrameMeta *frame_meta = (NvDsFrameMeta *) (l_frame->data);
    bus_count = 0;
    num_rects = 0;
    car_count = 0;
    for (l_obj = frame_meta->obj_meta_list; l_obj != nullptr; l_obj = l_obj->next) {
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
        for (NvDsMetaList *l_user_meta = obj_meta->obj_user_meta_list; l_user_meta != nullptr;
                l_user_meta = l_user_meta->next) {
          NvDsUserMeta *user_meta = (NvDsUserMeta *) (l_user_meta->data);
          if(user_meta->base_meta.meta_type == NVDS_USER_OBJ_META_NVDSANALYTICS)
          {
            NvDsAnalyticsObjInfo * user_meta_data = (NvDsAnalyticsObjInfo *)user_meta->user_meta_data;
            if (!user_meta_data->lcStatus.empty()){
              auto entry = objEntries.find(obj_meta->object_id);
              if (entry != objEntries.end()) {
                auto exit = getLCIdxFromString(user_meta_data->lcStatus[0]);
                crossings[entry->second][exit]+=1;
                std::cout << "Obj " << obj_meta->object_id << " exited" << std::endl;
              } else {
                objEntries.insert({obj_meta->object_id, getLCIdxFromString(user_meta_data->lcStatus[0])});
              }
            }
          }
        }
    }
    display_info_t displayInfo;
    if (fpsMsg != nullptr) {
      displayInfo.fps += std::string(fpsMsg);
    }
    std::stringstream out_string;
    /* Iterate user metadata in frames to search analytics metadata */
    for (NvDsMetaList * l_user = frame_meta->frame_user_meta_list; l_user != nullptr; l_user = l_user->next) {
        NvDsUserMeta *user_meta = (NvDsUserMeta *) l_user->data;
        if (user_meta->base_meta.meta_type != NVDS_USER_FRAME_META_NVDSANALYTICS) {
          continue;
        }
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
      }
      
      displayInfoToFrame(batch_meta, frame_meta, displayInfo);
      
      //std::cout << "Frame Number = " << frame_meta->frame_num << " of Stream = " << frame_meta->pad_index << ", Number of objects = " << num_rects <<
      //        " Bus Count = " << bus_count << " Car Count = " << car_count << " " << out_string.str().c_str() << std::endl;
  }
  return GST_PAD_PROBE_OK;
}

void printCrossingsMatrix() {
  std::cout << "  N NE SE SV NV" << std::endl;
  std::size_t idx = 0;
  for (const auto &entry: crossings) {
    std::cout << getLCFromIdx(idx++) << " ";
    for (const auto &exit: entry) {
      std::cout << exit << " ";
    }
    std::cout << std::endl;
  }
}

} // namespace metadata