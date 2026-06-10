#include <functional>

#include "libslic3r/Algorithm/LineSplit.hpp"
#include "libslic3r/ExtrusionEntity.hpp"
#include "libslic3r/ExtrusionEntityCollection.hpp"

#include "SurfaceModifier.hpp"

namespace Slic3r::Feature::SurfaceModifier {

// Returns true if any point of pts lies inside the painted zones.
static bool any_point_in_zones(const Points &pts, const ExPolygons &zones)
{
    for (const Point &p : pts)
        for (const ExPolygon &z : zones)
            if (z.contains(p))
                return true;
    return false;
}

// Split one extrusion path at the painted-zone boundary, appending the resulting
// sub-paths to `out`. Each sub-path is flagged surface_modifier_zone according to
// whether it lies inside the painted zone. Geometry (point positions) is preserved
// exactly; split_line only inserts new points where the path crosses the boundary.
static void split_path_by_zone(const ExtrusionPath &src, const ExPolygons &zones, ExtrusionPaths &out)
{
    const Points &pts = src.polyline.points;
    if (pts.size() < 2) {
        out.push_back(src);
        return;
    }

    const Algorithm::SplittedLine splitted = Algorithm::split_line(pts, zones, false);
    if (splitted.empty()) {
        // No boundary crossing: the path is entirely inside or entirely outside.
        ExtrusionPath p(src);
        p.surface_modifier_zone = any_point_in_zones(pts, zones);
        out.push_back(std::move(p));
        return;
    }

    // splitted holds N junctions = N-1 segments. Segment s (between junction s and
    // s+1) is inside the zone iff splitted[s].clipped. Group consecutive segments
    // of equal membership into one sub-path.
    const size_t num_seg = splitted.size() - 1;
    size_t       run_start = 0;
    bool         cur = splitted[0].clipped;
    for (size_t s = 1; s <= num_seg; ++s) {
        const bool flush = (s == num_seg) || (splitted[s].clipped != cur);
        if (!flush)
            continue;
        Points sub_pts;
        sub_pts.reserve(s - run_start + 1);
        for (size_t k = run_start; k <= s; ++k)
            sub_pts.push_back(splitted[k].p);
        if (sub_pts.size() >= 2) {
            ExtrusionPath p(src);
            p.polyline.points = std::move(sub_pts);
            p.polyline.fitting_result.clear();
            p.surface_modifier_zone = cur;
            out.push_back(std::move(p));
        }
        run_start = s;
        if (s < num_seg)
            cur = splitted[s].clipped;
    }
}

size_t apply_speed_zones(LayerRegionPtrs &regions, const ExPolygons &painted_zones)
{
    if (painted_zones.empty())
        return 0;

    size_t painted_subpaths = 0;

    // All perimeter roles are overridden by the G-code speed hook, including
    // overhang sections so the painted speed applies across overhangs too.
    const auto is_target_wall = [](ExtrusionRole role) {
        return is_perimeter(role);
    };

    // A LayerRegion's perimeters collection is NOT a flat list of loops: it holds
    // one ExtrusionEntityCollection per island, each containing the loops (and
    // possibly further nested collections / thin-wall paths). We must recurse.
    std::function<void(ExtrusionEntity *)> visit = [&](ExtrusionEntity *ee) {
        if (auto *coll = dynamic_cast<ExtrusionEntityCollection *>(ee)) {
            for (ExtrusionEntity *child : coll->entities)
                visit(child);
        } else if (auto *loop = dynamic_cast<ExtrusionLoop *>(ee)) {
            bool loop_has_target = false;
            for (const ExtrusionPath &p : loop->paths)
                if (is_target_wall(p.role())) { loop_has_target = true; break; }
            if (!loop_has_target)
                return;
            ExtrusionPaths new_paths;
            new_paths.reserve(loop->paths.size());
            for (const ExtrusionPath &p : loop->paths) {
                if (is_target_wall(p.role())) {
                    const size_t before = new_paths.size();
                    split_path_by_zone(p, painted_zones, new_paths);
                    for (size_t i = before; i < new_paths.size(); ++i)
                        if (new_paths[i].surface_modifier_zone)
                            ++painted_subpaths;
                } else {
                    new_paths.push_back(p);
                }
            }
            loop->paths = std::move(new_paths);
        }
    };

    for (LayerRegion *lr : regions)
        for (ExtrusionEntity *ee : lr->perimeters.entities)
            visit(ee);

    return painted_subpaths;
}

} // namespace Slic3r::Feature::SurfaceModifier
