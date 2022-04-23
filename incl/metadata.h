
#ifndef __VEHICLE_METADATA__ 
#define __VEHICLE_METADATA__

#include <gst/gst.h>
#include "types.h"

namespace metadata {

GstPadProbeReturn nvdsanalyticsSrcPadBufferProbe (GstPad *, GstPadProbeInfo *, gpointer);
void printCrossingsMatrix();

extern meta_producer_t producer;

} // namespace metadata

#endif //__VEHICLE_METADATA__