#include "df/enabler.h"
using df::global::enabler;

void fps_min_hook() {
    if (enabler->fps < 10)
        enabler->fps = 10;
}
