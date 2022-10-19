#include "layout.h"

#include "group.h"
#include "proxy.h"
#include "view.h"

namespace Karm::Ui {

struct Grow;

/* --- Empty ---------------------------------------------------------------- */

struct Empty : public View<Empty> {
    Math::Vec2i _size;

    Empty(Math::Vec2i size) : _size(size) {}

    void reconcile(Empty &o) override {
        _size = o._size;
    }

    Math::Vec2i size(Math::Vec2i, Layout::Hint) override {
        return _size;
    }

    void paint(Gfx::Context &g, Math::Recti) override {
        if (debugShowEmptyBounds) {
            auto b = bound();
            g._rect(b, Gfx::WHITE.withOpacity(0.01));
            g._line({b.topStart(), b.bottomEnd()}, Gfx::WHITE.withOpacity(0.01));
            g._line({b.topEnd(), b.bottomStart()}, Gfx::WHITE.withOpacity(0.01));
        }
    }
};

Child empty(Math::Vec2i size) {
    return makeStrong<Empty>(size);
}

Child cond(bool cond, Child child) {
    if (cond) {
        return child;
    } else {
        return empty();
    }
}

/* --- Bound ---------------------------------------------------------------- */

struct Bound : public Proxy<Bound> {
    Math::Recti _bound;

    Bound(Child child)
        : Proxy(child) {}

    Math::Recti bound() override {
        return _bound;
    }

    void layout(Math::Recti bound) override {
        _bound = bound;
        child().layout(bound);
    }

    Math::Vec2i size(Math::Vec2i s, Layout::Hint hint) override {
        return child().size(s, hint);
    }
};

Child bound(Child child) {
    return makeStrong<Bound>(child);
}

/* --- Separator ------------------------------------------------------------ */

struct Separator : public View<Separator> {
    Math::Vec2i size(Math::Vec2i, Layout::Hint) override {
        return {1};
    }

    void paint(Gfx::Context &g, Math::Recti) override {
        g.save();
        g.fillStyle(Gfx::ZINC700);
        g.fill(bound());
        g.restore();
    }
};

Child separator() {
    return makeStrong<Separator>();
}

/* --- Align ---------------------------------------------------------------- */

struct Align : public Proxy<Align> {
    Layout::Align _align;

    Align(Layout::Align align, Child child) : Proxy(child), _align(align) {}

    void layout(Math::Recti bound) override {
        auto childSize = child().size(bound.size(), _child.is<Grow>() ? Layout::Hint::MAX : Layout::Hint::MIN);
        child()
            .layout(_align.apply<int>(
                Layout::Flow::LEFT_TO_RIGHT,
                childSize,
                bound));
    };

    Math::Vec2i size(Math::Vec2i s, Layout::Hint hint) override {
        return _align.size(child().size(s, hint), s, hint);
    }
};

Child align(Layout::Align align, Child child) {
    return makeStrong<Align>(align, child);
}

Child center(Child child) {
    return align(Layout::Align::CENTER, child);
}

Child fit(Child child) {
    return align(Layout::Align::FIT, child);
}

Child cover(Child child) {
    return align(Layout::Align::COVER, child);
}

Child hcenter(Child child) {
    return align(Layout::Align::HCENTER | Layout::Align::TOP, child);
}

Child vcenter(Child child) {
    return align(Layout::Align::VCENTER | Layout::Align::START, child);
}

Child hcenterFill(Child child) {
    return align(Layout::Align::HCENTER | Layout::Align::VFILL, child);
}

Child vcenterFill(Child child) {
    return align(Layout::Align::VCENTER | Layout::Align::HFILL, child);
}

/* --- Sizing --------------------------------------------------------------- */

struct Sizing : public Proxy<Sizing> {

    Math::Vec2i _min;
    Math::Vec2i _max;
    Math::Recti _rect;

    Sizing(Math::Vec2i min, Math::Vec2i max, Child child)
        : Proxy(child), _min(min), _max(max) {}

    Math::Recti bound() override {
        return _rect;
    }

    void layout(Math::Recti bound) override {
        _rect = bound;
        child().layout(bound);
    }

    Math::Vec2i size(Math::Vec2i s, Layout::Hint hint) override {
        auto result = child().size(s, hint);

        if (_min.x != UNCONSTRAINED) {
            result.x = max(result.x, _min.x);
        }

        if (_min.y != UNCONSTRAINED) {
            result.y = max(result.y, _min.y);
        }

        if (_max.x != UNCONSTRAINED) {
            result.x = min(result.x, _max.x);
        }

        if (_max.y != UNCONSTRAINED) {
            result.y = min(result.y, _max.y);
        }

        return result;
    }
};

Child minSize(Math::Vec2i size, Child child) {
    return makeStrong<Sizing>(size, UNCONSTRAINED, child);
}

Child minSize(int size, Child child) {
    return minSize(Math::Vec2i{size}, child);
}

Child maxSize(Math::Vec2i size, Child child) {
    return makeStrong<Sizing>(UNCONSTRAINED, size, child);
}

Child maxSize(int size, Child child) {
    return maxSize(Math::Vec2i{size}, child);
}

Child pinSize(Math::Vec2i size, Child child) {
    return makeStrong<Sizing>(size, size, child);
}

Child pinSize(int size, Child child) {
    return minSize(Math::Vec2i{size}, child);
}

/* --- Spacing -------------------------------------------------------------- */

struct Spacing : public Proxy<Spacing> {
    Layout::Spacingi _spacing;

    Spacing(Layout::Spacingi spacing, Child child)
        : Proxy(child), _spacing(spacing) {}

    void reconcile(Spacing &o) override {
        _spacing = o._spacing;
        Proxy<Spacing>::reconcile(o);
    }

    void paint(Gfx::Context &g, Math::Recti r) override {
        child().paint(g, r);
        if (debugShowLayoutBounds) {
            g._rect(child().bound(), Gfx::LIME);
        }
    }

    void layout(Math::Recti rect) override {
        child().layout(_spacing.shrink(Layout::Flow::LEFT_TO_RIGHT, rect));
    }

    Math::Vec2i size(Math::Vec2i s, Layout::Hint hint) override {
        return child().size(s - _spacing.all(), hint) + _spacing.all();
    }

    Math::Recti bound() override {
        return _spacing.grow(Layout::Flow::LEFT_TO_RIGHT, child().bound());
    }
};

Child spacing(Layout::Spacingi s, Child child) {
    return makeStrong<Spacing>(s, child);
}

/* --- Aspect Ratio --------------------------------------------------------- */

struct AspectRatio : public Proxy<AspectRatio> {
    float _ratio;

    AspectRatio(float ratio, Child child)
        : Proxy(child), _ratio(ratio) {}

    void reconcile(AspectRatio &o) override {
        _ratio = o._ratio;
        Proxy<AspectRatio>::reconcile(o);
    }

    void paint(Gfx::Context &g, Math::Recti r) override {
        child().paint(g, r);
        if (debugShowLayoutBounds) {
            g._rect(child().bound(), Gfx::INDIGO);
        }
    }

    void layout(Math::Recti rect) override {
        child().layout(rect);
    }

    Math::Vec2i size(Math::Vec2i s, Layout::Hint hint) override {
        auto childSize = child().size(s, hint);
        auto childRatio = (float)childSize.x / (float)childSize.y;

        if (childRatio > _ratio) {
            return Math::Vec2i{
                (int)(s.y * _ratio),
                s.y,
            };
        } else {
            return Math::Vec2i{
                s.x,
                (int)(s.x / _ratio),
            };
        }
    }

    Math::Recti bound() override {
        return child().bound();
    }
};

Child aspectRatio(float ratio, Child child) {
    return makeStrong<AspectRatio>(ratio, child);
}

/* --- Stack ---------------------------------------------------------------- */

struct StackLayout : public Group<StackLayout> {
    using Group::Group;

    void layout(Math::Recti r) override {
        for (auto &child : children()) {
            child->layout(r);
        }
    }
};

Child stack(Children children) {
    return makeStrong<StackLayout>(children);
}

/* --- Dock ----------------------------------------------------------------- */

struct DockItem : public Proxy<DockItem> {
    Layout::Dock _dock;

    DockItem(Layout::Dock dock, Child child) : Proxy(child), _dock(dock) {}

    Layout::Dock dock() const { return _dock; }
};

Child docked(Layout::Dock dock, Child child) {
    return makeStrong<DockItem>(dock, child);
}

Child dockTop(Child child) {
    return docked(Layout::Dock::TOP, child);
}

Child dockBottom(Child child) {
    return docked(Layout::Dock::BOTTOM, child);
}

Child dockStart(Child child) {
    return docked(Layout::Dock::START, child);
}

Child dockEnd(Child child) {
    return docked(Layout::Dock::END, child);
}

struct DockLayout : public Group<DockLayout> {
    using Group::Group;

    static auto getDock(auto &child) -> Layout::Dock {
        if (child.template is<DockItem>()) {
            return child.template unwrap<DockItem>().dock();
        }

        return Layout::Dock::NONE;
    };

    void layout(Math::Recti bound) override {
        _bound = bound;
        auto outer = bound;

        for (auto &child : children()) {
            Math::Recti inner = child->size(outer.size(), Layout::Hint::MIN);
            child->layout(getDock(child).apply(inner, outer));
        }
    }

    static Math::Vec2i apply(Layout::Orien o, Math::Vec2i current, Math::Vec2i inner) {
        switch (o) {
        case Layout::Orien::NONE:
            current = current.max(inner);

        case Layout::Orien::HORIZONTAL:
            current.x += inner.x;
            current.y = max(current.y, inner.y);
            break;

        case Layout::Orien::VERTICAL:
            current.x = max(current.x, inner.x);
            current.y += inner.y;
            break;
        default:
            break;
        }

        return current;
    }

    Math::Vec2i size(Math::Vec2i s, Layout::Hint hint) override {
        Math::Vec2i currentSize{};
        for (auto &child : mutIterRev(children())) {
            currentSize = apply(getDock(child).orien(), child->size(currentSize, Layout::Hint::MIN), currentSize);
        }

        if (hint == Layout::Hint::MAX) {
            currentSize = currentSize.max(s);
        }

        return currentSize;
    }
};

Child dock(Children children) {
    return makeStrong<DockLayout>(children);
}

/* --- Flow ----------------------------------------------------------------- */

struct Grow : public Proxy<Grow> {
    int _grow;

    Grow(Child child) : Proxy(child), _grow(1) {}

    Grow(int grow, Child child) : Proxy(child), _grow(grow) {}

    int grow() const {
        return _grow;
    }
};

Child grow(Child child) {
    return makeStrong<Grow>(child);
}

Child grow(int grow, Child child) {
    return makeStrong<Grow>(grow, child);
}

Child grow(int g) {
    return grow(g, empty());
}

struct FlowLayout : public Group<FlowLayout> {
    using Group::Group;

    FlowStyle _style;

    FlowLayout(FlowStyle style, Children children)
        : Group(children), _style(style) {}

    int computeGrowUnit(Math::Recti r) {
        int total = 0;
        int grows = 0;

        for (auto &child : children()) {
            if (child.is<Grow>()) {
                grows += child.unwrap<Grow>().grow();
            } else {
                total += _style.flow.getX(child->size(r.size(), Layout::Hint::MIN));
            }
        }

        int all = _style.flow.getWidth(r) - _style.gaps * (max(1uz, children().len()) - 1);
        int growTotal = max(0, all - total);
        return (growTotal) / max(1, grows);
    }

    void layout(Math::Recti r) override {
        _bound = r;

        int growUnit = computeGrowUnit(r);
        int start = _style.flow.getStart(r);

        for (auto &child : children()) {
            Math::Recti inner = {};
            auto childSize = child->size(r.size(), Layout::Hint::MIN);

            inner = _style.flow.setStart(inner, start);
            if (child.is<Grow>()) {
                inner = _style.flow.setWidth(inner, growUnit * child.unwrap<Grow>().grow());
            } else {
                inner = _style.flow.setWidth(inner, _style.flow.getX(childSize));
            }

            inner = _style.flow.setTop(inner, _style.flow.getTop(r));
            inner = _style.flow.setBottom(inner, _style.flow.getBottom(r));

            child->layout(_style.align.apply(_style.flow, Math::Recti{childSize}, inner));
            start += _style.flow.getWidth(inner) + _style.gaps;
        }
    }

    Math::Vec2i size(Math::Vec2i s, Layout::Hint hint) override {
        int w{};
        int h{hint == Layout::Hint::MAX ? _style.flow.getY(s) : 0};
        bool grow = false;

        for (auto &child : children()) {
            if (child.is<Grow>())
                grow = true;

            auto childSize = child->size(s, Layout::Hint::MIN);
            w += _style.flow.getX(childSize);
            h = max(h, _style.flow.getY(childSize));
        }

        w += _style.gaps * (max(1uz, children().len()) - 1);
        if (grow && hint == Layout::Hint::MAX) {
            w = max(_style.flow.getX(s), w);
        }

        return _style.flow.orien() == Layout::Orien::HORIZONTAL
                   ? Math::Vec2i{w, h}
                   : Math::Vec2i{h, w};
    }
};

Child flow(FlowStyle style, Children children) {
    return makeStrong<FlowLayout>(style, children);
}

/* --- Grid ----------------------------------------------------------------- */

struct Cell : public Proxy<Cell> {
    Math::Vec2i _start{};
    Math::Vec2i _end{};

    Cell(Math::Vec2i start, Math::Vec2i end, Child child)
        : Proxy(child), _start(start), _end(end) {}

    Math::Vec2i start() const {
        return _start;
    }

    Math::Vec2i end() const {
        return _end;
    }
};

Child cell(Math::Vec2i pos, Child child) {
    return makeStrong<Cell>(pos, pos, child);
}

Child cell(Math::Vec2i start, Math::Vec2i end, Child child) {
    return makeStrong<Cell>(start, end, child);
}

struct GridLayout : public Group<GridLayout> {
    struct _Dim {
        int start;
        int size;
        int end() const {
            return start + size;
        }
    };

    GridStyle _style;
    Vec<_Dim> _rows;
    Vec<_Dim> _columns;

    GridLayout(GridStyle style, Children children)
        : Group(children), _style(style) {}

    int computeGrowUnitRows(Math::Recti r) {
        int total = 0;
        int grows = 0;

        for (auto &row : _style.rows) {
            if (row.unit == GridUnit::GROW) {
                grows += row.value;
            } else {
                total += row.value;
            }
        }

        int all = _style.flow.getHeight(r) - _style.gaps.y * (max(1uz, _style.rows.len()) - 1);
        int growTotal = max(0, all - total);
        return (growTotal) / max(1, grows);
    }

    int computeGrowUnitColumns(Math::Recti r) {
        int total = 0;
        int grows = 0;

        for (auto &column : _style.columns) {
            if (column.unit == GridUnit::GROW) {
                grows += column.value;
            } else {
                total += column.value;
            }
        }

        int all = _style.flow.getWidth(r) - _style.gaps.x * (max(1uz, _style.columns.len()) - 1);
        int growTotal = max(0, all - total);
        return (growTotal) / max(1, grows);
    }

    void layout(Math::Recti r) override {
        _bound = r;

        // compute the dimensions of the grid
        _rows.clear();
        _columns.clear();

        int growUnitRows = computeGrowUnitRows(r);
        int growUnitColumns = computeGrowUnitColumns(r);

        int row = _style.flow.getTop(r);
        int column = _style.flow.getStart(r);

        for (auto &r : _style.rows) {
            if (r.unit == GridUnit::GROW) {
                _rows.pushBack({_Dim{row, growUnitRows * r.value}});
                row += growUnitRows * r.value;
            } else {
                _rows.pushBack({_Dim{row, r.value}});
                row += r.value;
            }

            row += _style.gaps.y;
        }

        for (auto &c : _style.columns) {
            if (c.unit == GridUnit::GROW) {
                _columns.pushBack({_Dim{column, growUnitColumns * c.value}});
                column += growUnitColumns * c.value;
            } else {
                _columns.pushBack({_Dim{column, c.value}});
                column += c.value;
            }

            column += _style.gaps.x;
        }

        // layout the children
        int index = 0;
        for (auto &child : children()) {
            if (child.is<Cell>()) {
                auto &cell = child.unwrap<Cell>();

                auto start = cell.start();
                auto end = cell.end();

                auto startRow = _rows[start.y];
                auto startColumn = _columns[start.x];

                auto endRow = _rows[end.y];
                auto endColumn = _columns[end.x];

                auto childRect = Math::Recti{
                    startColumn.start,
                    startRow.start,
                    endColumn.end() - startColumn.start,
                    endRow.end() - startRow.start,
                };

                child->layout(childRect);
                index = end.y * _columns.len() + end.x;
            } else {
                auto row = index / _columns.len();
                auto column = index % _columns.len();

                auto startRow = _rows[row];
                auto startColumn = _columns[column];

                auto childRect = Math::Recti{
                    startColumn.start,
                    startRow.start,
                    startColumn.size,
                    startRow.size,
                };

                child->layout(childRect);
            }
            index++;
        }
    }

    Math::Vec2i size(Math::Vec2i s, Layout::Hint hint) override {
        int growUnitRows = computeGrowUnitRows(Math::Recti{0, s});
        int growUnitColumns = computeGrowUnitColumns(Math::Recti{0, s});

        int row = 0;
        bool rowGrow = false;

        int column = 0;
        bool columnGrow = false;

        for (auto &r : _style.rows) {
            if (r.unit == GridUnit::GROW) {
                row += growUnitRows * r.value;
                rowGrow = true;
            } else {
                row += r.value;
            }
        }

        for (auto &c : _style.columns) {
            if (c.unit == GridUnit::GROW) {
                column += growUnitColumns * c.value;
                columnGrow = true;
            } else {
                column += c.value;
            }
        }

        if (rowGrow && hint == Layout::Hint::MAX) {
            row = max(_style.flow.getY(s), row);
        }

        if (columnGrow && hint == Layout::Hint::MAX) {
            column = max(_style.flow.getX(s), column);
        }

        return Math::Vec2i{column, row};
    }
};

Child grid(GridStyle style, Children children) {
    return makeStrong<GridLayout>(style, children);
}

} // namespace Karm::Ui
