#include <cmath>

#include "../ClipperUtils.hpp"
#include "../ExPolygon.hpp"
#include "../Surface.hpp"

#include "FillSingleLine.hpp"

namespace Slic3r {

void FillSingleLine::_fill_surface_single(
    const FillParams              &params,
    unsigned int                   /* thickness_layers */,
    const std::pair<float, Point> & /* direction */,
    ExPolygon                      expolygon,
    Polylines                     &polylines_out)
{
    const float angle_rad = params.single_line_angle * float(M_PI) / 180.f;

    // Centre of the object bounding box shifted by the user-specified offset.
    Point center = this->bounding_box.center();
    center += Point(scale_(params.single_line_offset_x),
                    scale_(params.single_line_offset_y));

    // Half-diagonal of the expolygon bounding box — guarantees the raw line
    // reaches both sides regardless of orientation.
    BoundingBox bbox = expolygon.contour.bounding_box();
    const coord_t half_diag = coord_t(std::ceil(
        std::sqrt(double(bbox.size()(0)) * bbox.size()(0) +
                  double(bbox.size()(1)) * bbox.size()(1))
        / 2.0)) + scale_(1.0);

    const float dx = std::cos(angle_rad);
    const float dy = std::sin(angle_rad);

    Polyline candidate;
    candidate.points.push_back(Point(center(0) - coord_t(half_diag * dx),
                                     center(1) - coord_t(half_diag * dy)));
    candidate.points.push_back(Point(center(0) + coord_t(half_diag * dx),
                                     center(1) + coord_t(half_diag * dy)));

    Polylines clipped = intersection_pl({candidate}, expolygon);
    for (Polyline &pl : clipped)
        polylines_out.emplace_back(std::move(pl));
}

} // namespace Slic3r
