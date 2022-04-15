#ifndef __VEHICLE_TRACKING_TYPES__
#define __VEHICLE_TRACKING_TYPES__

#include <glib.h>
#include <gst/gst.h>

namespace vehicletracking {

using arg_count_t = int;
using arg_var_t = char **;
using bus_id_t = guint;
using loop_t = GMainLoop *;
using pipeline_t = GstElement *;

} // namespace vehicletracking

#endif


