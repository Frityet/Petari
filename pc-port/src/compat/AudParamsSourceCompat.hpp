#pragma once

#include "Game/AudioLib/AudParams.hpp"

// The original compiler treats nullptr as an integer zero macro. Only the
// recovered parameter table needs that spelling for its integer ARAM address.
// Include its dependencies before defining the macro so host headers retain
// the native C++ keyword.
#define nullptr 0
