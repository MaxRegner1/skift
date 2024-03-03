#pragma once

#pragma once

#include <karm-ui/view.h>
#include <mixbox/mixbox.h>
#include <web-css/colors.h>

#include "base.h"

namespace Hideo::Demos {

static inline Demo MIXBOX_DEMO{
    Mdi::GRADIENT_HORIZONTAL,
    "Mixbox",
    "Pigment Acurate Color Mixing",
    [] {
        return Ui::canvas(
            [](Gfx::Context &g, Math::Vec2i bound) {
                auto grad = Gfx::Gradient::linear();
                for (f64 i = 0; i <= 100; i += 10)
                    grad.withStop(Mixbox::lerpColor(Web::Css::YELLOW, Web::Css::RED, i / 100.0), i / 100.0);
                g.fillStyle(grad.bake());
                g.fill(bound);
            });
    },
};

} // namespace Hideo::Demos