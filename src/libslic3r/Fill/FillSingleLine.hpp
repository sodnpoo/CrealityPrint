#ifndef slic3r_FillSingleLine_hpp_
#define slic3r_FillSingleLine_hpp_

#include "FillBase.hpp"

namespace Slic3r {

class FillSingleLine : public Fill
{
public:
    Fill* clone() const override { return new FillSingleLine(*this); }
    ~FillSingleLine() override = default;
    bool is_self_crossing() override { return false; }
    bool has_consistent_pattern() const override { return true; }

protected:
    void _fill_surface_single(
        const FillParams              &params,
        unsigned int                   thickness_layers,
        const std::pair<float, Point> &direction,
        ExPolygon                      expolygon,
        Polylines                     &polylines_out) override;
};

} // namespace Slic3r
#endif // slic3r_FillSingleLine_hpp_
