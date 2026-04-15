#pragma once

#include <optional>
#include <string>
#include <vector>

#include "online_board/domain/core/enums.hpp"

namespace online_board::domain {

struct Point {
    double x {0.0};
    double y {0.0};

    auto operator<=>(const Point&) const = default;
};

struct Size {
    double width {0.0};
    double height {0.0};

    auto operator<=>(const Size&) const = default;
};

struct Rect {
    double x {0.0};
    double y {0.0};
    double width {0.0};
    double height {0.0};

    auto operator<=>(const Rect&) const = default;
};

struct StrokePayload {
    std::string color;
    double thickness {1.0};
    std::vector<Point> points;
    bool finished {false};
};

struct ShapePayload {
    ShapeType shape_type {ShapeType::rectangle};
    Rect geometry;
    std::string stroke_color;
    std::optional<std::string> fill_color;
    double stroke_width {1.0};
};

struct TextPayload {
    Point position;
    Size size;
    std::string text;
    std::string font_family;
    double font_size {12.0};
    std::string color;
};

}  // namespace online_board::domain
