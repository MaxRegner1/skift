#include <karm-ui/drag.h>

namespace Karm::Ui {

/* --- Dismisable ----------------------------------------------------------- */

struct Dismisable :
    public ProxyNode<Dismisable> {

    OnDismis _onDismis;
    DismisDir _dir;
    f64 _threshold;
    Eased2f _drag{};
    Math::Vec2i _last{};
    bool _dismissed{};

    Dismisable(OnDismis onDismis, DismisDir dir, f64 threshold, Ui::Child child)
        : ProxyNode(child),
          _onDismis(std::move(onDismis)),
          _dir(dir),
          _threshold(threshold) {}

    Math::Vec2i drag() const {
        return _drag.value().cast<isize>();
    }

    void paint(Gfx::Context &g, Math::Recti r) override {
        g.save();

        g.clip(bound());
        g.origin(drag());
        r.xy = r.xy - drag();
        child().paint(g, r);

        g.restore();
    }

    void event(Events::Event &e) override {
        auto oldBound = bound().clipTo(child().bound().offset(_last));
        if (_drag.needRepaint(*this, e)) {
            auto newBound = bound().clipTo(child().bound().offset(drag()));
            _last = drag();
            Ui::shouldRepaint(*this, oldBound.mergeWith(newBound));
        }

        if (_dismissed and _drag.reached()) {
            _onDismis(*this);
        }

        Ui::ProxyNode<Dismisable>::event(e);
    }

    void bubble(Events::Event &e) override {
        if (e.is<DragEvent>()) {
            auto &de = e.unwrap<DragEvent>();

            if (de.type == DragEvent::DRAG) {
                auto d = _drag.target() + de.delta;

                d.x = clamp(
                    d.x,
                    (bool)(_dir & DismisDir::LEFT) ? -bound().width : 0,
                    (bool)(_dir & DismisDir::RIGHT) ? bound().width : 0);

                d.y = clamp(
                    d.y,
                    (bool)(_dir & DismisDir::TOP) ? -bound().height : 0,
                    (bool)(_dir & DismisDir::DOWN) ? bound().height : 0);

                _drag.set(*this, d);
            } else if (de.type == DragEvent::END) {
                if ((bool)(_dir & DismisDir::HORIZONTAL)) {
                    if (Math::abs(_drag.targetX()) / (f64)bound().width > _threshold) {
                        _drag.animate(*this, {bound().width * (_drag.targetX() < 0.0 ? -1.0 : 1), 0}, 0.25, Math::Easing::cubicOut);
                        _dismissed = true;
                    } else {
                        _drag.animate(*this, {0, _drag.targetY()}, 0.25, Math::Easing::exponentialOut);
                    }
                }
                if ((bool)(_dir & DismisDir::VERTICAL)) {
                    if (Math::abs(_drag.targetY()) / (f64)bound().height > _threshold) {
                        _drag.animate(*this, {0, bound().height * (_drag.targetY() < 0.0 ? -1.0 : 1)}, 0.25, Math::Easing::cubicOut);
                        _dismissed = true;
                    } else {
                        _drag.animate(*this, {_drag.targetX(), 0}, 0.25, Math::Easing::exponentialOut);
                    }
                }
            }
        } else {
            Ui::ProxyNode<Dismisable>::bubble(e);
        }
    }
};

Child dismisable(OnDismis onDismis, DismisDir dir, f64 threshold, Ui::Child child) {
    return makeStrong<Dismisable>(std::move(onDismis), dir, threshold, std::move(child));
}

/* --- Drag Region ---------------------------------------------------------- */

struct DragRegion : public ProxyNode<DragRegion> {
    bool _grabbed{};

    using ProxyNode::ProxyNode;

    void event(Events::Event &e) override {
        if (not _grabbed)
            _child->event(e);

        if (e.accepted)
            return;

        e.handle<Events::MouseEvent>([&](auto &m) {
            if (not bound().contains(m.pos) and not _grabbed) {
                return false;
            }

            if (m.type == Events::MouseEvent::PRESS) {
                _grabbed = true;

                DragEvent de = {.type = DragEvent::START};
                bubble(de);

                return true;
            }

            if (m.type == Events::MouseEvent::RELEASE) {
                _grabbed = false;

                DragEvent de = {.type = DragEvent::END};
                bubble(de);

                return true;
            }

            if (m.type == Events::MouseEvent::MOVE) {
                if (_grabbed) {
                    DragEvent de = {.type = DragEvent::DRAG, .delta = m.delta};
                    bubble(de);

                    return true;
                }
            }

            return false;
        });
    }
};

Child dragRegion(Child child) {
    return makeStrong<DragRegion>(child);
}

/* --- Handle --------------------------------------------------------------- */

Child handle() {
    return empty({128, 4}) |
           box({
               .borderRadius = 999,
               .backgroundPaint = GRAY50,
           }) |
           spacing(12) |
           center() |
           minSize({UNCONSTRAINED, 48});
}

Child dragHandle() {
    return handle() | dragRegion();
}

Child buttonHandle(OnPress press) {
    return handle() |
           button(std::move(press), Ui::ButtonStyle::none());
}

} // namespace Karm::Ui
