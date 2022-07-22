#pragma once

#include "rect.h"
#include "vec.h"

namespace Karm::Math {

template <typename T>
union Edge {
    struct {
        Vec2<T> start{}, end{};
    };

    struct {
        T sx, sy, ex, ey;
    };

    T _els[4];

    constexpr Edge() : _els{0, 0, 0, 0} {};

    constexpr Edge(Vec2<T> start, Vec2<T> end) : start(start), end(end) {}

    constexpr Edge(T x1, T y1, T x2, T y2) : start(x1, y1), end(x2, y2) {}

    constexpr Rect<T> bound() const {
        return Rect<T>::fromTwoPoint(start, end);
    }

    constexpr Vec2<T> dir() const {
        return end - start;
    }

    constexpr T len() const {
        return dir().len();
    }

    constexpr T operator[](int i) const {
        return _els[i];
    }

    template <typename U>
    constexpr Edge<U> cast() const {
        return {
            start.template cast<U>(),
            end.template cast<U>(),
        };
    }
};

using Edgei = Edge<int>;

using Edgef = Edge<double>;

} // namespace Karm::Math

namespace Karm::Fmt {

template <typename T>
struct Formatter<Math::Edge<T>> {
    Result<size_t> format(Io::_TextWriter &writer, Math::Edge<T> edge) {
        return Fmt::format(writer, "Edge({}, {}, {}, {})", edge.sx, edge.sy, edge.ex, edge.ey);
    }
};

} // namespace Karm::Fmt
