#pragma once

#include <asx/reactor.hpp>

namespace ingress {
   void init(float min_threshold, asx::reactor::Handle on_threshold_alert);
   float get_measurement();
}