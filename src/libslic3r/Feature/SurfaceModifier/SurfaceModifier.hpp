#ifndef libslic3r_SurfaceModifier_hpp_
#define libslic3r_SurfaceModifier_hpp_

#include "libslic3r/ExPolygon.hpp"
#include "libslic3r/Layer.hpp"

namespace Slic3r::Feature::SurfaceModifier {

// Splits external-perimeter loops at the painted-zone boundaries and flags the
// inside sub-paths (ExtrusionPath::surface_modifier_zone) so the G-code speed
// override applies to exactly the painted span (outer wall only for now).
// Geometry is preserved; split_line only inserts new points where a loop crosses
// the zone boundary. Returns the number of painted sub-paths produced.
size_t apply_speed_zones(LayerRegionPtrs &regions, const ExPolygons &painted_zones);

} // namespace Slic3r::Feature::SurfaceModifier

#endif // libslic3r_SurfaceModifier_hpp_
