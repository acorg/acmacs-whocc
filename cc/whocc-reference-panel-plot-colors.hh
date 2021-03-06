#pragma once

#include "acmacs-base/color.hh"
#include "acmacs-chart-2/chart.hh"

constexpr const size_t sNumberOfAllTiters = 325;

extern const acmacs::chart::Titer sAllTiters[sNumberOfAllTiters];

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif

const Color transparent{"transparent"}, homologous_background{"grey95"};

#pragma GCC diagnostic pop

extern const uint32_t sMedianTiterColors[sNumberOfAllTiters][sNumberOfAllTiters];

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
