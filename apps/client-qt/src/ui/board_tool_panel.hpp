#pragma once

#include <QColor>
#include <QFrame>

#include "board_canvas_widget.hpp"

class QSlider;
class QToolButton;

namespace online_board::client_qt {

class BoardToolPanel final : public QFrame {
public:
    explicit BoardToolPanel(QWidget* parent = nullptr);

    [[nodiscard]] QToolButton* pen_button() const { return pen_button_; }
    [[nodiscard]] QToolButton* rectangle_button() const { return rectangle_button_; }
    [[nodiscard]] QToolButton* text_button() const { return text_button_; }
    [[nodiscard]] QToolButton* eraser_button() const { return eraser_button_; }
    [[nodiscard]] QToolButton* color_button() const { return color_button_; }
    [[nodiscard]] QToolButton* zoom_in_button() const { return zoom_in_button_; }
    [[nodiscard]] QToolButton* zoom_out_button() const { return zoom_out_button_; }
    [[nodiscard]] QToolButton* reset_view_button() const { return reset_view_button_; }
    [[nodiscard]] QSlider* stroke_width_slider() const { return stroke_width_slider_; }

    void set_active_tool(BoardCanvasWidget::Tool tool);
    void set_controls_enabled(bool enabled);
    void set_selected_color(const QColor& color);
    void set_stroke_width(int width);
    void place_default();

    [[nodiscard]] int stroke_width() const;

private:
    static void configure_panel_button(QToolButton* button);

    QToolButton* pen_button_ {nullptr};
    QToolButton* rectangle_button_ {nullptr};
    QToolButton* text_button_ {nullptr};
    QToolButton* eraser_button_ {nullptr};
    QToolButton* color_button_ {nullptr};
    QToolButton* zoom_in_button_ {nullptr};
    QToolButton* zoom_out_button_ {nullptr};
    QToolButton* reset_view_button_ {nullptr};
    QSlider* stroke_width_slider_ {nullptr};
    class QLabel* stroke_width_value_label_ {nullptr};
};

}  // namespace online_board::client_qt
