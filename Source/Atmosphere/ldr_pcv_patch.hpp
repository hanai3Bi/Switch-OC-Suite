// placed in Atmosphere/stratosphere/loader/source/

#pragma once
#include <stratosphere.hpp>

namespace ams::ldr {

    void ApplyPcvPatch(u8 *mapped_module, size_t mapped_size, int i);

    void ApplyCtestPatch(u8 *mapped_module, size_t mapped_size, int i);

    void ApplyCopyrightPatch(u8 *mapped_module, size_t mapped_size, int i);

}
