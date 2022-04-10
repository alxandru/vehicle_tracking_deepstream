
#ifndef __VEHICLE_METADATA__ 
#define __VEHICLE_METADATA__

#include <gst/gst.h>

namespace metadata {

GstPadProbeReturn nvdsanalyticsSrcPadBufferProbe (GstPad *, GstPadProbeInfo *, gpointer);

} // namespace metadata

#endif //__VEHICLE_METADATA__