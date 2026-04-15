#include "board_tool_panel.hpp"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

namespace online_board::client_qt {
namespace {

void refresh_widget_style(QWidget* widget) {
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

}  // namespace

BoardToolPanel::BoardToolPanel(QWidget* parent)
    : QFrame(parent) {
    setObjectName(QStringLiteral("floatingPanel"));

    auto* floating_layout = new QVBoxLayout(this);
    floating_layout->setContentsMargins(12, 12, 12, 12);
    floating_layout->setSpacing(8);

    pen_button_ = new QToolButton(this);
    pen_button_->setText(QStringLiteral("Pen"));
    pen_button_->setCheckable(true);
    pen_button_->setObjectName(QStringLiteral("floatingPanelButton"));

    rectangle_button_ = new QToolButton(this);
    rectangle_button_->setText(QStringLiteral("Rectangle"));
    rectangle_button_->setCheckable(true);
    rectangle_button_->setObjectName(QStringLiteral("floatingPanelButton"));

    text_button_ = new QToolButton(this);
    text_button_->setText(QStringLiteral("Text"));
    text_button_->setCheckable(true);
    text_button_->setObjectName(QStringLiteral("floatingPanelButton"));

    eraser_button_ = new QToolButton(this);
    eraser_button_->setText(QStringLiteral("Eraser"));
    eraser_button_->setCheckable(true);
    eraser_button_->setObjectName(QStringLiteral("floatingPanelButton"));

    auto* tool_group = new QButtonGroup(this);
    tool_group->setExclusive(true);
    tool_group->addButton(pen_button_);
    tool_group->addButton(rectangle_button_);
    tool_group->addButton(text_button_);
    tool_group->addButton(eraser_button_);

    configure_panel_button(pen_button_);
    configure_panel_button(rectangle_button_);
    configure_panel_button(text_button_);
    configure_panel_button(eraser_button_);

    floating_layout->addWidget(pen_button_);
    floating_layout->addWidget(rectangle_button_);
    floating_layout->addWidget(text_button_);
    floating_layout->addWidget(eraser_button_);

    color_button_ = new QToolButton(this);
    color_button_->setText(QStringLiteral("Color"));
    color_button_->setObjectName(QStringLiteral("colorOverlayButton"));
    configure_panel_button(color_button_);
    floating_layout->addWidget(color_button_);

    auto* thickness_label = new QLabel(QStringLiteral("Stroke"), this);
    thickness_label->setObjectName(QStringLiteral("miniPanelLabel"));
    floating_layout->addWidget(thickness_label);

    auto* stroke_row = new QHBoxLayout();
    stroke_row->setContentsMargins(0, 0, 0, 0);
    stroke_row->setSpacing(8);
    stroke_width_slider_ = new QSlider(Qt::Horizontal, this);
    stroke_width_slider_->setRange(1, 10);
    stroke_width_slider_->setValue(3);
    stroke_width_slider_->setObjectName(QStringLiteral("strokeWidthSlider"));
    stroke_width_value_label_ = new QLabel(QStringLiteral("3 px"), this);
    stroke_width_value_label_->setObjectName(QStringLiteral("miniPanelValue"));
    stroke_width_value_label_->setMinimumWidth(34);
    stroke_row->addWidget(stroke_width_slider_, 1);
    stroke_row->addWidget(stroke_width_value_label_);
    floating_layout->addLayout(stroke_row);

    auto* divider = new QFrame(this);
    divider->setFrameShape(QFrame::HLine);
    divider->setFrameShadow(QFrame::Plain);
    divider->setObjectName(QStringLiteral("divider"));
    floating_layout->addWidget(divider);

    zoom_in_button_ = new QToolButton(this);
    zoom_in_button_->setText(QStringLiteral("+"));
    zoom_in_button_->setObjectName(QStringLiteral("floatingPanelButton"));

    zoom_out_button_ = new QToolButton(this);
    zoom_out_button_->setText(QStringLiteral("-"));
    zoom_out_button_->setObjectName(QStringLiteral("floatingPanelButton"));

    reset_view_button_ = new QToolButton(this);
    reset_view_button_->setText(QStringLiteral("100%"));
    reset_view_button_->setObjectName(QStringLiteral("floatingPanelButton"));

    configure_panel_button(zoom_in_button_);
    configure_panel_button(zoom_out_button_);
    configure_panel_button(reset_view_button_);
    floating_layout->addWidget(zoom_in_button_);
    floating_layout->addWidget(zoom_out_button_);
    floating_layout->addWidget(reset_view_button_);
    floating_layout->addStretch(1);

    QObject::connect(stroke_width_slider_, &QSlider::valueChanged, this, [this](int value) {
        stroke_width_value_label_->setText(QStringLiteral("%1 px").arg(value));
    });
}

void BoardToolPanel::set_active_tool(BoardCanvasWidget::Tool tool) {
    pen_button_->setChecked(tool == BoardCanvasWidget::Tool::pen);
    rectangle_button_->setChecked(tool == BoardCanvasWidget::Tool::rectangle);
    text_button_->setChecked(tool == BoardCanvasWidget::Tool::text);
    eraser_button_->setChecked(tool == BoardCanvasWidget::Tool::eraser);
}

void BoardToolPanel::set_controls_enabled(bool enabled) {
    pen_button_->setEnabled(enabled);
    rectangle_button_->setEnabled(enabled);
    text_button_->setEnabled(enabled);
    eraser_button_->setEnabled(enabled);
    color_button_->setEnabled(enabled);
    zoom_in_button_->setEnabled(enabled);
    zoom_out_button_->setEnabled(enabled);
    reset_view_button_->setEnabled(enabled);
    stroke_width_slider_->setEnabled(enabled);
}

void BoardToolPanel::set_selected_color(const QColor& color) {
    const auto text_color = color.lightnessF() < 0.45 ? QStringLiteral("#fffaf2") : QStringLiteral("#1d2730");
    color_button_->setStyleSheet(QStringLiteral(
        "QToolButton#colorOverlayButton {"
        "background: %1;"
        "color: %2;"
        "border: 1px solid #dac8b0;"
        "}").arg(color.name(QColor::HexRgb), text_color));
}

void BoardToolPanel::set_stroke_width(int width) {
    stroke_width_slider_->setValue(width);
}

void BoardToolPanel::place_default() {
    setFixedWidth(164);
    adjustSize();
    move(16, 16);
    raise();
}

int BoardToolPanel::stroke_width() const {
    return stroke_width_slider_->value();
}

void BoardToolPanel::configure_panel_button(QToolButton* button) {
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

}  // namespace online_board::client_qt
