#include "main_window.hpp"

#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "main_window_shared.hpp"

namespace online_board::client_qt {

void MainWindow::setup_ui() {
    auto* central = new QWidget(this);
    central->setObjectName(QStringLiteral("appRoot"));

    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    page_stack_ = new QStackedWidget(central);
    page_stack_->setObjectName(QStringLiteral("pageStack"));
    root->addWidget(page_stack_);

    build_auth_page();
    build_cabinet_page();
    build_board_page();

    setCentralWidget(central);
    resize(1480, 920);
    setWindowTitle(QStringLiteral("Dosochka"));

    sync_create_board_controls();
}

void MainWindow::build_auth_page() {
    auto* auth_page = new QWidget(page_stack_);
    auth_page->setObjectName(QStringLiteral("authPage"));
    auto* auth_layout = new QVBoxLayout(auth_page);
    auth_layout->setContentsMargins(24, 24, 24, 24);
    auth_layout->setSpacing(18);

    auto* auth_shell = new QWidget(auth_page);
    auth_shell->setMaximumWidth(760);
    auto* auth_shell_layout = new QVBoxLayout(auth_shell);
    auth_shell_layout->setContentsMargins(0, 0, 0, 0);
    auth_shell_layout->setSpacing(16);

    auto* auth_brand = new QLabel(QStringLiteral("Dosochka"), auth_shell);
    auth_brand->setObjectName(QStringLiteral("pageTitle"));
    auth_shell_layout->addWidget(auth_brand, 0, Qt::AlignHCenter);

    auto* auth_intro = new QLabel(
        QStringLiteral("Login first, then open the cabinet, and only after that move into the board."),
        auth_shell);
    auth_intro->setObjectName(QStringLiteral("pageSubtitle"));
    auth_intro->setWordWrap(true);
    auth_intro->setAlignment(Qt::AlignHCenter);
    auth_shell_layout->addWidget(auth_intro);

    auto auth_card = create_card(
        QStringLiteral("Login window"),
        QStringLiteral("Connect to the Boost server, then use an account or enter as guest."),
        auth_shell);

    host_edit_ = new QLineEdit(QStringLiteral("127.0.0.1"), auth_card.frame);
    host_edit_->setPlaceholderText(QStringLiteral("Server host"));
    host_edit_->setClearButtonEnabled(true);
    port_spin_ = new QSpinBox(auth_card.frame);
    port_spin_->setRange(1, 65535);
    port_spin_->setValue(4000);
    connect_button_ = new QPushButton(QStringLiteral("Connect"), auth_card.frame);
    connect_button_->setObjectName(QStringLiteral("primaryButton"));

    auto* connection_grid = new QGridLayout();
    connection_grid->setHorizontalSpacing(10);
    connection_grid->setVerticalSpacing(10);
    connection_grid->addWidget(new QLabel(QStringLiteral("Host"), auth_card.frame), 0, 0);
    connection_grid->addWidget(new QLabel(QStringLiteral("Port"), auth_card.frame), 0, 2);
    connection_grid->addWidget(host_edit_, 1, 0, 1, 2);
    connection_grid->addWidget(port_spin_, 1, 2);
    connection_grid->addWidget(connect_button_, 2, 0, 1, 3);
    auth_card.content_layout->addLayout(connection_grid);

    auth_status_label_ = new QLabel(QStringLiteral("Connect to the server to continue"), auth_card.frame);
    auth_status_label_->setObjectName(QStringLiteral("inlineStatus"));
    auth_status_label_->setProperty("state", QStringLiteral("neutral"));
    auth_card.content_layout->addWidget(auth_status_label_);
    auth_card.content_layout->addWidget(create_divider(auth_card.frame));

    login_edit_ = new QLineEdit(auth_card.frame);
    login_edit_->setPlaceholderText(QStringLiteral("Login"));
    login_edit_->setClearButtonEnabled(true);
    display_name_edit_ = new QLineEdit(auth_card.frame);
    display_name_edit_->setPlaceholderText(QStringLiteral("Display name for registration"));
    display_name_edit_->setClearButtonEnabled(true);
    password_edit_ = new QLineEdit(auth_card.frame);
    password_edit_->setPlaceholderText(QStringLiteral("Password"));
    password_edit_->setEchoMode(QLineEdit::Password);
    guest_edit_ = new QLineEdit(auth_card.frame);
    guest_edit_->setPlaceholderText(QStringLiteral("Guest nickname"));
    guest_edit_->setClearButtonEnabled(true);

    auto* auth_form = new QFormLayout();
    auth_form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    auth_form->setHorizontalSpacing(10);
    auth_form->setVerticalSpacing(10);
    auth_form->addRow(QStringLiteral("Login"), login_edit_);
    auth_form->addRow(QStringLiteral("Display name"), display_name_edit_);
    auth_form->addRow(QStringLiteral("Password"), password_edit_);
    auth_card.content_layout->addLayout(auth_form);

    auto* auth_actions = new QHBoxLayout();
    auth_actions->setSpacing(10);
    register_button_ = new QPushButton(QStringLiteral("Register"), auth_card.frame);
    register_button_->setObjectName(QStringLiteral("secondaryButton"));
    login_button_ = new QPushButton(QStringLiteral("Login"), auth_card.frame);
    login_button_->setObjectName(QStringLiteral("primaryButton"));
    auth_actions->addWidget(register_button_);
    auth_actions->addWidget(login_button_);
    auth_card.content_layout->addLayout(auth_actions);

    auth_card.content_layout->addWidget(create_divider(auth_card.frame));

    auto* guest_label = new QLabel(QStringLiteral("Guest access"), auth_card.frame);
    guest_label->setObjectName(QStringLiteral("sectionLabel"));
    auth_card.content_layout->addWidget(guest_label);

    auto* guest_row = new QHBoxLayout();
    guest_row->setSpacing(10);
    guest_button_ = new QPushButton(QStringLiteral("Enter as guest"), auth_card.frame);
    guest_button_->setObjectName(QStringLiteral("ghostButton"));
    guest_row->addWidget(guest_edit_, 1);
    guest_row->addWidget(guest_button_);
    auth_card.content_layout->addLayout(guest_row);

    auth_shell_layout->addWidget(auth_card.frame);
    auth_layout->addStretch(1);
    auth_layout->addWidget(auth_shell, 0, Qt::AlignHCenter);
    auth_layout->addStretch(1);
    page_stack_->addWidget(auth_page);
}

void MainWindow::build_cabinet_page() {
    auto* cabinet_page = new QWidget(page_stack_);
    cabinet_page->setObjectName(QStringLiteral("cabinetPage"));
    auto* cabinet_layout = new QVBoxLayout(cabinet_page);
    cabinet_layout->setContentsMargins(20, 20, 20, 20);
    cabinet_layout->setSpacing(14);

    auto cabinet_header = create_card(
        QStringLiteral("Board cabinet"),
        QStringLiteral("Create a board or join an existing one. The board itself opens on a separate full-screen view."),
        cabinet_page);
    auto* cabinet_header_row = new QHBoxLayout();
    auto* cabinet_text_column = new QVBoxLayout();
    auto* cabinet_heading = new QLabel(QStringLiteral("Welcome back"), cabinet_header.frame);
    cabinet_heading->setObjectName(QStringLiteral("sectionHeading"));
    cabinet_text_column->addWidget(cabinet_heading);
    cabinet_account_label_ = new QLabel(QStringLiteral("Anonymous"), cabinet_header.frame);
    cabinet_account_label_->setObjectName(QStringLiteral("pageSubtitle"));
    cabinet_text_column->addWidget(cabinet_account_label_);
    cabinet_header_row->addLayout(cabinet_text_column, 1);

    auto* cabinet_badges = new QVBoxLayout();
    cabinet_status_label_ = new QLabel(QStringLiteral("Awaiting authentication"), cabinet_header.frame);
    cabinet_status_label_->setObjectName(QStringLiteral("inlineStatus"));
    cabinet_status_label_->setProperty("state", QStringLiteral("neutral"));
    cabinet_badges->addWidget(cabinet_status_label_, 0, Qt::AlignRight);

    logout_button_ = new QPushButton(QStringLiteral("Log out"), cabinet_header.frame);
    logout_button_->setObjectName(QStringLiteral("ghostButton"));
    cabinet_badges->addWidget(logout_button_, 0, Qt::AlignRight);
    cabinet_badges->setSpacing(10);

    cabinet_header_row->addLayout(cabinet_badges);
    cabinet_header.content_layout->addLayout(cabinet_header_row);
    cabinet_layout->addWidget(cabinet_header.frame);

    auto boards = create_card(
        QStringLiteral("Boards"),
        QStringLiteral("This is the cabinet screen. Pick where to go next."),
        cabinet_page);

    auto* create_label = new QLabel(QStringLiteral("Create new board"), boards.frame);
    create_label->setObjectName(QStringLiteral("sectionLabel"));
    boards.content_layout->addWidget(create_label);

    create_title_edit_ = new QLineEdit(boards.frame);
    create_title_edit_->setPlaceholderText(QStringLiteral("Example: Sprint review"));
    create_title_edit_->setClearButtonEnabled(true);

    create_access_mode_combo_ = new QComboBox(boards.frame);
    create_access_mode_combo_->addItem(QStringLiteral("Private"), QStringLiteral("private_board"));
    create_access_mode_combo_->addItem(QStringLiteral("Password protected"), QStringLiteral("password_protected"));

    create_guest_policy_combo_ = new QComboBox(boards.frame);
    create_guest_policy_combo_->addItem(QStringLiteral("Guests disabled"), QStringLiteral("guest_disabled"));
    create_guest_policy_combo_->addItem(QStringLiteral("Guests view only"), QStringLiteral("guest_view_only"));
    create_guest_policy_combo_->addItem(QStringLiteral("Guests can edit"), QStringLiteral("guest_edit_allowed"));

    create_password_edit_ = new QLineEdit(boards.frame);
    create_password_edit_->setPlaceholderText(QStringLiteral("Password for protected boards"));
    create_password_edit_->setEchoMode(QLineEdit::Password);

    auto* create_form = new QFormLayout();
    create_form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    create_form->setHorizontalSpacing(10);
    create_form->setVerticalSpacing(10);
    create_form->addRow(QStringLiteral("Title"), create_title_edit_);
    create_form->addRow(QStringLiteral("Access"), create_access_mode_combo_);
    create_form->addRow(QStringLiteral("Guest policy"), create_guest_policy_combo_);
    create_form->addRow(QStringLiteral("Password"), create_password_edit_);
    boards.content_layout->addLayout(create_form);

    create_board_button_ = new QPushButton(QStringLiteral("Create board"), boards.frame);
    create_board_button_->setObjectName(QStringLiteral("primaryButton"));
    boards.content_layout->addWidget(create_board_button_);

    boards.content_layout->addWidget(create_divider(boards.frame));

    auto* join_label = new QLabel(QStringLiteral("Join existing board"), boards.frame);
    join_label->setObjectName(QStringLiteral("sectionLabel"));
    boards.content_layout->addWidget(join_label);

    join_board_id_edit_ = new QLineEdit(boards.frame);
    join_board_id_edit_->setPlaceholderText(QStringLiteral("Board id"));
    join_board_id_edit_->setClearButtonEnabled(true);
    join_password_edit_ = new QLineEdit(boards.frame);
    join_password_edit_->setPlaceholderText(QStringLiteral("Password if required"));
    join_password_edit_->setEchoMode(QLineEdit::Password);

    auto* join_form = new QFormLayout();
    join_form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    join_form->setHorizontalSpacing(10);
    join_form->setVerticalSpacing(10);
    join_form->addRow(QStringLiteral("Board id"), join_board_id_edit_);
    join_form->addRow(QStringLiteral("Password"), join_password_edit_);
    boards.content_layout->addLayout(join_form);

    join_board_button_ = new QPushButton(QStringLiteral("Join board"), boards.frame);
    join_board_button_->setObjectName(QStringLiteral("secondaryButton"));
    boards.content_layout->addWidget(join_board_button_);

    boards.content_layout->addWidget(create_divider(boards.frame));

    auto* list_label = new QLabel(QStringLiteral("Your boards"), boards.frame);
    list_label->setObjectName(QStringLiteral("sectionLabel"));
    boards.content_layout->addWidget(list_label);

    user_boards_list_ = new QListWidget(boards.frame);
    user_boards_list_->setSelectionMode(QAbstractItemView::SingleSelection);
    user_boards_list_->setUniformItemSizes(true);
    user_boards_list_->setMinimumHeight(180);
    boards.content_layout->addWidget(user_boards_list_);

    auto* board_actions = new QHBoxLayout();
    board_actions->setSpacing(10);
    open_selected_board_button_ = new QPushButton(QStringLiteral("Open selected"), boards.frame);
    open_selected_board_button_->setObjectName(QStringLiteral("primaryButton"));
    delete_selected_board_button_ = new QPushButton(QStringLiteral("Delete selected"), boards.frame);
    delete_selected_board_button_->setObjectName(QStringLiteral("ghostButton"));
    board_actions->addWidget(open_selected_board_button_);
    board_actions->addWidget(delete_selected_board_button_);
    boards.content_layout->addLayout(board_actions);

    cabinet_layout->addWidget(boards.frame);
    cabinet_layout->addStretch(1);
    page_stack_->addWidget(cabinet_page);
}

void MainWindow::build_board_page() {
    auto* board_page = new QWidget(page_stack_);
    board_page->setObjectName(QStringLiteral("boardPage"));
    auto* board_layout = new QVBoxLayout(board_page);
    board_layout->setContentsMargins(12, 12, 12, 12);
    board_layout->setSpacing(8);

    board_top_bar_ = new BoardTopBar(board_page);
    board_layout->addWidget(board_top_bar_);

    auto* canvas_card = new QFrame(board_page);
    canvas_card->setObjectName(QStringLiteral("canvasCard"));
    auto* canvas_layout = new QVBoxLayout(canvas_card);
    canvas_layout->setContentsMargins(0, 0, 0, 0);
    canvas_layout->setSpacing(0);
    canvas_ = new BoardCanvasWidget(canvas_card);
    canvas_->setMinimumSize(320, 240);
    canvas_layout->addWidget(canvas_);

    tool_panel_ = new BoardToolPanel(canvas_card);
    tool_panel_->raise();
    board_layout->addWidget(canvas_card, 1);

    page_stack_->addWidget(board_page);
}

}  // namespace online_board::client_qt
