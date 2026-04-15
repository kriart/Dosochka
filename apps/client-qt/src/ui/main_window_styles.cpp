#include "main_window.hpp"

namespace online_board::client_qt {

void MainWindow::apply_styles() {
    setStyleSheet(R"(
        QWidget#appRoot {
            background: #efe7d7;
            color: #1d2730;
            font-family: "Segoe UI";
            font-size: 13px;
        }

        QWidget#authPage, QWidget#cabinetPage, QWidget#boardPage {
            background: #efe7d7;
        }

        QFrame#card, QFrame#canvasCard, QFrame#topBar {
            background: #fffaf3;
            border: 1px solid #ddcfbd;
            border-radius: 16px;
        }

        QFrame#floatingPanel {
            background: rgba(255, 250, 243, 0.94);
            border: 1px solid #ddcfbd;
            border-radius: 16px;
        }

        QLabel#pageTitle {
            color: #243a45;
            font-family: "Bahnschrift SemiBold";
            font-size: 36px;
        }

        QLabel#pageSubtitle {
            color: #58656f;
            font-size: 13px;
        }

        QLabel#sectionHeading {
            color: #243a45;
            font-family: "Bahnschrift SemiBold";
            font-size: 18px;
        }

        QLabel#cardTitle, QLabel#sectionLabel {
            color: #845b3d;
            font-family: "Bahnschrift SemiBold";
            font-size: 14px;
        }

        QLabel#cardSubtitle, QLabel#cardNote {
            color: #6a737b;
            font-size: 12px;
        }

        QLabel#miniPanelLabel {
            color: #845b3d;
            font-family: "Bahnschrift SemiBold";
            font-size: 11px;
            padding-top: 2px;
        }

        QLabel#miniPanelValue {
            color: #58656f;
            font-family: "Bahnschrift SemiBold";
            font-size: 11px;
        }

        QLabel#boardTitleLight {
            color: #243a45;
            font-family: "Bahnschrift SemiBold";
            font-size: 22px;
        }

        QLabel#boardMetaLight {
            color: #6a737b;
            font-size: 13px;
        }

        QLabel#participantsCaption {
            color: #58656f;
            font-size: 12px;
            padding: 0 2px;
        }

        QToolButton#participantsSummaryButton {
            background: #f5ebdc;
            color: #243a45;
            border: 1px solid #dac8b0;
            border-radius: 12px;
            padding: 7px 12px;
            font-family: "Bahnschrift SemiBold";
            font-size: 12px;
        }

        QToolButton#participantsSummaryButton:hover {
            background: #ede1d0;
        }

        QLabel#inlineStatus {
            padding: 7px 12px;
            border-radius: 12px;
            font-family: "Bahnschrift SemiBold";
            font-size: 12px;
            min-height: 18px;
        }

        QLabel#inlineStatus[state="neutral"] {
            background: #ece3d5;
            color: #4d5a63;
            border: 1px solid #d8cab7;
        }

        QLabel#inlineStatus[state="success"] {
            background: #e2f0df;
            color: #2d6640;
            border: 1px solid #b6d2bc;
        }

        QLabel#inlineStatus[state="error"] {
            background: #f6e1df;
            color: #8b3e38;
            border: 1px solid #e0b9b4;
        }

        QLabel#dialogTitle {
            color: #1d2730;
            font-family: "Bahnschrift SemiBold";
            font-size: 18px;
        }

        QLabel#dialogSubtitle {
            color: #65717b;
            font-size: 12px;
        }

        QLineEdit, QComboBox, QSpinBox, QPlainTextEdit {
            background: #f7f0e5;
            border: 1px solid #d6c7b3;
            border-radius: 10px;
            padding: 5px 10px 6px 10px;
            min-height: 22px;
            selection-background-color: #d06f38;
            selection-color: white;
        }

        QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QPlainTextEdit:focus {
            border: 1px solid #d06f38;
            background: #fff7ee;
        }

        QPlainTextEdit {
            min-height: 96px;
        }

        QPushButton, QToolButton {
            border-radius: 12px;
            padding: 10px 14px;
            font-family: "Bahnschrift SemiBold";
            font-size: 13px;
        }

        QPushButton#primaryButton {
            background: #243a45;
            color: #fffaf2;
            border: none;
        }

        QPushButton#primaryButton:hover {
            background: #2d4957;
        }

        QPushButton#secondaryButton {
            background: #d06f38;
            color: #fffaf2;
            border: none;
        }

        QPushButton#secondaryButton:hover {
            background: #db7e4b;
        }

        QPushButton#ghostButton {
            background: #fffaf3;
            color: #243a45;
            border: 1px solid #cdbda8;
        }

        QPushButton#ghostButton:hover {
            background: #f1e7d8;
        }

        QToolButton#floatingPanelButton, QToolButton#colorOverlayButton {
            background: rgba(255, 250, 243, 0.95);
            color: #243a45;
            border: 1px solid #dac8b0;
            min-width: 0px;
            min-height: 38px;
            max-height: 38px;
            font-size: 12px;
            text-align: center;
        }

        QToolButton#floatingPanelButton:hover, QToolButton#colorOverlayButton:hover {
            background: #ede1d0;
        }

        QToolButton#floatingPanelButton:checked {
            background: #243a45;
            color: #fff9f1;
            border: 1px solid #243a45;
        }

        QToolButton#closeBoardButton {
            background: #f5ebdc;
            color: #243a45;
            border: 1px solid #dac8b0;
            min-width: 38px;
            max-width: 38px;
            min-height: 38px;
            max-height: 38px;
            border-radius: 19px;
            font-size: 16px;
            font-family: "Bahnschrift SemiBold";
        }

        QToolButton#closeBoardButton:hover {
            background: #ede1d0;
        }

        QPushButton:disabled, QToolButton:disabled, QLineEdit:disabled, QComboBox:disabled, QSpinBox:disabled {
            color: #938775;
        }

        QFrame#divider {
            color: #e3d6c7;
            background: #e3d6c7;
            min-height: 1px;
            max-height: 1px;
        }

        QSlider#strokeWidthSlider::groove:horizontal {
            background: #dfd0bc;
            height: 4px;
            border-radius: 2px;
        }

        QSlider#strokeWidthSlider::handle:horizontal {
            background: #243a45;
            width: 14px;
            margin: -6px 0;
            border-radius: 7px;
        }
    )");
}

}  // namespace online_board::client_qt
