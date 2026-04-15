#include "main_window.hpp"

#include <utility>
#include <vector>

#include <QColorDialog>
#include <QMessageBox>

#include "board_operation_builder.hpp"
#include "main_window_shared.hpp"

namespace online_board::client_qt {

void MainWindow::bind_callbacks() {
    controller_.on_status_changed = [this](const QString& text, const QString& state) {
        set_status_text(text, state);
    };
    controller_.on_account_changed = [this](const QString& text) {
        set_account_text(text);
    };
    controller_.on_show_auth_page = [this]() {
        show_auth_page();
    };
    controller_.on_show_cabinet_page = [this]() {
        show_cabinet_page();
    };
    controller_.on_show_board_page = [this]() {
        show_board_page();
    };
    controller_.on_session_state_changed = [this]() {
        sync_ui_state();
    };
    controller_.on_join_board_id_suggested = [this](const QString& board_id) {
        join_board_id_edit_->setText(board_id);
        controller_.join_board(board_id, create_password_edit_->text());
    };
    controller_.on_board_context_changed = [this](const QString& title, const QString& subtitle, const QString& board_id) {
        current_board_id_ = board_id;
        set_board_context(title, subtitle);
    };
    controller_.on_board_opened = [this](const domain::BoardSnapshot& snapshot, domain::BoardRole role) {
        canvas_->set_role(role);
        canvas_->set_snapshot(snapshot);
    };
    controller_.on_board_snapshot_changed = [this](const domain::BoardSnapshot& snapshot) {
        canvas_->set_snapshot(snapshot);
        set_board_context(current_board_title_, current_board_subtitle_);
    };
    controller_.on_user_boards_changed = [this](const std::vector<ClientSessionController::UserBoardEntry>& boards) {
        update_user_boards(boards);
    };
    controller_.on_participants_changed = [this](const QStringList& participants) {
        board_top_bar_->set_participants(participants);
    };
    controller_.on_board_cleared = [this]() {
        reset_board_state();
    };
    controller_.on_warning_requested = [this](const QString& title, const QString& message) {
        QMessageBox::warning(this, title, message);
    };

    QObject::connect(connect_button_, &QPushButton::clicked, this, [this]() { ensure_connected(); });
    QObject::connect(register_button_, &QPushButton::clicked, this, [this]() {
        controller_.register_user(login_edit_->text(), display_name_edit_->text(), password_edit_->text());
    });
    QObject::connect(login_button_, &QPushButton::clicked, this, [this]() {
        controller_.login(login_edit_->text(), password_edit_->text());
    });
    QObject::connect(guest_button_, &QPushButton::clicked, this, [this]() {
        controller_.enter_guest(guest_edit_->text());
    });

    QObject::connect(create_board_button_, &QPushButton::clicked, this, [this]() {
        auto guest_policy = create_guest_policy_combo_->currentData().toString();
        const auto access_mode = create_access_mode_combo_->currentData().toString();
        if (access_mode != QStringLiteral("password_protected")) {
            guest_policy = QStringLiteral("guest_disabled");
        }
        controller_.create_board(create_title_edit_->text(), access_mode, guest_policy, create_password_edit_->text());
    });
    QObject::connect(join_board_button_, &QPushButton::clicked, this, [this]() {
        controller_.join_board(join_board_id_edit_->text(), join_password_edit_->text());
    });
    QObject::connect(user_boards_list_, &QListWidget::itemSelectionChanged, this, [this]() {
        sync_user_board_actions();
    });
    QObject::connect(open_selected_board_button_, &QPushButton::clicked, this, [this]() {
        const auto* item = user_boards_list_->currentItem();
        if (item == nullptr) {
            return;
        }
        const auto board_id = item->data(Qt::UserRole).toString();
        join_board_id_edit_->setText(board_id);
        controller_.join_board(board_id, {});
    });
    QObject::connect(delete_selected_board_button_, &QPushButton::clicked, this, [this]() {
        const auto* item = user_boards_list_->currentItem();
        if (item == nullptr) {
            return;
        }
        const auto board_id = item->data(Qt::UserRole).toString();
        const auto title = item->data(Qt::UserRole + 1).toString();
        const auto confirmed = QMessageBox::question(
            this,
            QStringLiteral("Delete board"),
            QStringLiteral("Delete board \"%1\" permanently?").arg(title));
        if (confirmed != QMessageBox::Yes) {
            return;
        }
        controller_.delete_board(board_id);
    });
    QObject::connect(logout_button_, &QPushButton::clicked, this, [this]() { logout(); });
    QObject::connect(board_top_bar_->leave_button(), &QToolButton::clicked, this, [this]() { leave_board(); });
    QObject::connect(
        create_access_mode_combo_,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        [this](int) {
            sync_create_board_controls();
            sync_ui_state();
        });

    QObject::connect(tool_panel_->pen_button(), &QToolButton::clicked, this, [this]() { set_active_tool(BoardCanvasWidget::Tool::pen); });
    QObject::connect(tool_panel_->rectangle_button(), &QToolButton::clicked, this, [this]() { set_active_tool(BoardCanvasWidget::Tool::rectangle); });
    QObject::connect(tool_panel_->text_button(), &QToolButton::clicked, this, [this]() { set_active_tool(BoardCanvasWidget::Tool::text); });
    QObject::connect(tool_panel_->eraser_button(), &QToolButton::clicked, this, [this]() { set_active_tool(BoardCanvasWidget::Tool::eraser); });

    canvas_->on_rectangle_created = [this](const QRectF& rect) {
        enqueue_operation(BoardOperationBuilder::make_rectangle(
            controller_.next_object_id(),
            controller_.next_z_index(),
            rect,
            current_color_,
            tool_panel_->stroke_width()));
    };

    canvas_->on_text_requested = [this](const QPointF& point) {
        const auto text = request_text_input(this);
        if (!text.has_value()) {
            return;
        }

        enqueue_operation(BoardOperationBuilder::make_text(
            controller_.next_object_id(),
            controller_.next_z_index(),
            point,
            *text,
            current_color_));
    };

    canvas_->on_object_erase_requested = [this](const common::ObjectId& object_id) {
        enqueue_operation(BoardOperationBuilder::make_delete_object(object_id));
    };

    canvas_->on_stroke_finished = [this](const std::vector<QPointF>& points) {
        const auto object_id = controller_.next_object_id();
        const auto z_index = controller_.next_z_index();
        for (auto operation : BoardOperationBuilder::make_stroke_sequence(
                 object_id,
                 z_index,
                 points,
                 current_color_,
                 tool_panel_->stroke_width())) {
            enqueue_operation(std::move(operation));
        }
    };

    QObject::connect(tool_panel_->color_button(), &QToolButton::clicked, this, [this]() {
        const auto color = QColorDialog::getColor(current_color_, this, QStringLiteral("Choose drawing color"));
        if (!color.isValid()) {
            return;
        }

        current_color_ = color;
        canvas_->set_primary_color(current_color_);
        update_color_button();
    });

    QObject::connect(tool_panel_->zoom_in_button(), &QToolButton::clicked, this, [this]() { canvas_->zoom_in(); });
    QObject::connect(tool_panel_->zoom_out_button(), &QToolButton::clicked, this, [this]() { canvas_->zoom_out(); });
    QObject::connect(tool_panel_->reset_view_button(), &QToolButton::clicked, this, [this]() { canvas_->reset_zoom(); });
}

void MainWindow::ensure_connected() {
    controller_.connect_to_server(host_edit_->text(), static_cast<quint16>(port_spin_->value()));
    sync_ui_state();
}

void MainWindow::enqueue_operation(QJsonObject operation) {
    controller_.enqueue_operation(std::move(operation));
}

void MainWindow::sync_create_board_controls() {
    const auto access_mode = create_access_mode_combo_->currentData().toString();
    const bool is_password_protected = access_mode == QStringLiteral("password_protected");
    create_guest_policy_combo_->setEnabled(is_password_protected);
    create_password_edit_->setEnabled(is_password_protected);
    if (!is_password_protected) {
        create_guest_policy_combo_->setCurrentIndex(0);
        create_password_edit_->clear();
    }
}

}  // namespace online_board::client_qt
