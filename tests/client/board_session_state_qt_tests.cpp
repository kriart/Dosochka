#include <QtTest/QtTest>

#include <QMenu>
#include <QSlider>
#include <QToolButton>

#include "board_canvas_scene_growth.hpp"
#include "board_canvas_widget.hpp"
#include "board_participants_view.hpp"
#include "board_session_state.hpp"
#include "board_tool_panel.hpp"

namespace online_board::client_qt {

class BoardSessionStateQtTests final : public QObject {
    Q_OBJECT

private slots:
    void DifferentClientStatesGenerateDistinctObjectIdsOnSameBoard();
    void EraserHitTestReturnsTopmostObject();
    void EraserDragRequestsDeletionForEachTouchedObject();
    void ViewerRoleDoesNotEmitEditCallbacks();
    void ParticipantsViewUpdatesSummaryAndPopupEntries();
    void ToolPanelReflectsSelectedToolAndEnabledState();
    void SceneGrowthLockPreventsRepeatedExpansionUntilTargetLeavesEdge();
};

void BoardSessionStateQtTests::DifferentClientStatesGenerateDistinctObjectIdsOnSameBoard() {
    domain::BoardSnapshot snapshot;
    snapshot.board_id = common::BoardId{"shared-board"};

    BoardSessionState first_state;
    BoardSessionState second_state;

    first_state.attach(snapshot.board_id, domain::BoardRole::editor, snapshot);
    second_state.attach(snapshot.board_id, domain::BoardRole::editor, snapshot);

    const auto first_object_id = first_state.next_object_id();
    const auto second_object_id = second_state.next_object_id();

    QVERIFY(!first_object_id.isEmpty());
    QVERIFY(!second_object_id.isEmpty());
    QVERIFY(first_object_id != second_object_id);
}

void BoardSessionStateQtTests::EraserHitTestReturnsTopmostObject() {
    domain::BoardSnapshot snapshot;
    snapshot.board_id = common::BoardId{"shared-board"};

    snapshot.objects.emplace(
        common::ObjectId{"shape-bottom"},
        domain::BoardObject{
            .object_id = common::ObjectId{"shape-bottom"},
            .board_id = snapshot.board_id,
            .type = domain::ObjectType::shape,
            .created_by = domain::GuestPrincipal{.guest_id = "g1", .nickname = "guest"},
            .created_at = common::Timestamp{},
            .updated_at = common::Timestamp{},
            .is_deleted = false,
            .z_index = 1,
            .payload = domain::ShapePayload{
                .shape_type = domain::ShapeType::rectangle,
                .geometry = domain::Rect{.x = 20.0, .y = 20.0, .width = 160.0, .height = 120.0},
                .stroke_color = "#000000",
                .fill_color = std::string{"#cccccc"},
                .stroke_width = 2.0,
            },
        });

    snapshot.objects.emplace(
        common::ObjectId{"shape-top"},
        domain::BoardObject{
            .object_id = common::ObjectId{"shape-top"},
            .board_id = snapshot.board_id,
            .type = domain::ObjectType::shape,
            .created_by = domain::GuestPrincipal{.guest_id = "g2", .nickname = "guest2"},
            .created_at = common::Timestamp{},
            .updated_at = common::Timestamp{},
            .is_deleted = false,
            .z_index = 2,
            .payload = domain::ShapePayload{
                .shape_type = domain::ShapeType::rectangle,
                .geometry = domain::Rect{.x = 40.0, .y = 40.0, .width = 160.0, .height = 120.0},
                .stroke_color = "#000000",
                .fill_color = std::string{"#999999"},
                .stroke_width = 2.0,
            },
        });

    BoardCanvasWidget canvas;
    canvas.set_role(domain::BoardRole::editor);
    canvas.set_snapshot(snapshot);

    const auto object = canvas.object_at(QPointF(80.0, 80.0));
    QVERIFY(object.has_value());
    QCOMPARE(QString::fromStdString(object->value), QStringLiteral("shape-top"));
}

void BoardSessionStateQtTests::EraserDragRequestsDeletionForEachTouchedObject() {
    domain::BoardSnapshot snapshot;
    snapshot.board_id = common::BoardId{"shared-board"};

    snapshot.objects.emplace(
        common::ObjectId{"shape-left"},
        domain::BoardObject{
            .object_id = common::ObjectId{"shape-left"},
            .board_id = snapshot.board_id,
            .type = domain::ObjectType::shape,
            .created_by = domain::GuestPrincipal{.guest_id = "g1", .nickname = "guest"},
            .created_at = common::Timestamp{},
            .updated_at = common::Timestamp{},
            .is_deleted = false,
            .z_index = 1,
            .payload = domain::ShapePayload{
                .shape_type = domain::ShapeType::rectangle,
                .geometry = domain::Rect{.x = 20.0, .y = 20.0, .width = 100.0, .height = 100.0},
                .stroke_color = "#000000",
                .fill_color = std::string{"#cccccc"},
                .stroke_width = 2.0,
            },
        });

    snapshot.objects.emplace(
        common::ObjectId{"shape-right"},
        domain::BoardObject{
            .object_id = common::ObjectId{"shape-right"},
            .board_id = snapshot.board_id,
            .type = domain::ObjectType::shape,
            .created_by = domain::GuestPrincipal{.guest_id = "g2", .nickname = "guest2"},
            .created_at = common::Timestamp{},
            .updated_at = common::Timestamp{},
            .is_deleted = false,
            .z_index = 2,
            .payload = domain::ShapePayload{
                .shape_type = domain::ShapeType::rectangle,
                .geometry = domain::Rect{.x = 180.0, .y = 20.0, .width = 100.0, .height = 100.0},
                .stroke_color = "#000000",
                .fill_color = std::string{"#999999"},
                .stroke_width = 2.0,
            },
        });

    BoardCanvasWidget canvas;
    canvas.resize(420, 220);
    canvas.set_role(domain::BoardRole::editor);
    canvas.set_tool(BoardCanvasWidget::Tool::eraser);
    canvas.set_snapshot(snapshot);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    QStringList erased;
    canvas.on_object_erase_requested = [&erased](const common::ObjectId& object_id) {
        erased.push_back(QString::fromStdString(object_id.value));
    };

    const QPoint left_point = canvas.mapFromScene(QPointF(60.0, 60.0));
    const QPoint right_point = canvas.mapFromScene(QPointF(220.0, 60.0));
    const QPoint far_right_point = canvas.mapFromScene(QPointF(260.0, 60.0));

    QTest::mousePress(canvas.viewport(), Qt::LeftButton, Qt::NoModifier, left_point);
    QTest::mouseMove(canvas.viewport(), right_point, 10);
    QTest::mouseRelease(canvas.viewport(), Qt::LeftButton, Qt::NoModifier, far_right_point);

    QCOMPARE(erased.size(), 2);
    QCOMPARE(erased.at(0), QStringLiteral("shape-left"));
    QCOMPARE(erased.at(1), QStringLiteral("shape-right"));
}

void BoardSessionStateQtTests::ViewerRoleDoesNotEmitEditCallbacks() {
    BoardCanvasWidget canvas;
    canvas.resize(320, 240);
    canvas.set_role(domain::BoardRole::viewer);
    canvas.set_tool(BoardCanvasWidget::Tool::rectangle);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    bool rectangle_created = false;
    canvas.on_rectangle_created = [&rectangle_created](const QRectF&) {
        rectangle_created = true;
    };

    const QPoint start = canvas.mapFromScene(QPointF(40.0, 40.0));
    const QPoint finish = canvas.mapFromScene(QPointF(160.0, 140.0));

    QTest::mousePress(canvas.viewport(), Qt::LeftButton, Qt::NoModifier, start);
    QTest::mouseMove(canvas.viewport(), finish, 10);
    QTest::mouseRelease(canvas.viewport(), Qt::LeftButton, Qt::NoModifier, finish);

    QVERIFY(!rectangle_created);
}

void BoardSessionStateQtTests::ParticipantsViewUpdatesSummaryAndPopupEntries() {
    BoardParticipantsView view;

    auto* summary_button = view.findChild<QToolButton*>(QStringLiteral("participantsSummaryButton"));
    QVERIFY(summary_button != nullptr);
    QVERIFY(summary_button->menu() != nullptr);

    QCOMPARE(summary_button->text(), QStringLiteral("0 online"));
    QVERIFY(!summary_button->isEnabled());
    QCOMPARE(summary_button->toolTip(), QStringLiteral("No active participants"));
    QCOMPARE(summary_button->menu()->actions().size(), 1);
    QCOMPARE(summary_button->menu()->actions().front()->text(), QStringLiteral("No active participants"));
    QVERIFY(!summary_button->menu()->actions().front()->isEnabled());

    view.set_entries({QStringLiteral("Owner"), QStringLiteral("Guest One")});

    QCOMPARE(summary_button->text(), QStringLiteral("2 online"));
    QVERIFY(summary_button->isEnabled());
    QCOMPARE(summary_button->toolTip(), QStringLiteral("Owner\nGuest One"));
    QCOMPARE(summary_button->menu()->actions().size(), 2);
    QCOMPARE(summary_button->menu()->actions().at(0)->text(), QStringLiteral("Owner"));
    QCOMPARE(summary_button->menu()->actions().at(1)->text(), QStringLiteral("Guest One"));
    QVERIFY(!summary_button->menu()->actions().at(0)->isEnabled());
    QVERIFY(!summary_button->menu()->actions().at(1)->isEnabled());
}

void BoardSessionStateQtTests::ToolPanelReflectsSelectedToolAndEnabledState() {
    BoardToolPanel panel;

    panel.set_active_tool(BoardCanvasWidget::Tool::text);
    QVERIFY(!panel.pen_button()->isChecked());
    QVERIFY(!panel.rectangle_button()->isChecked());
    QVERIFY(panel.text_button()->isChecked());
    QVERIFY(!panel.eraser_button()->isChecked());

    panel.set_controls_enabled(false);
    QVERIFY(!panel.pen_button()->isEnabled());
    QVERIFY(!panel.rectangle_button()->isEnabled());
    QVERIFY(!panel.text_button()->isEnabled());
    QVERIFY(!panel.eraser_button()->isEnabled());
    QVERIFY(!panel.color_button()->isEnabled());
    QVERIFY(!panel.zoom_in_button()->isEnabled());
    QVERIFY(!panel.zoom_out_button()->isEnabled());
    QVERIFY(!panel.reset_view_button()->isEnabled());
    QVERIFY(!panel.stroke_width_slider()->isEnabled());

    panel.set_controls_enabled(true);
    panel.set_stroke_width(7);
    QCOMPARE(panel.stroke_width(), 7);
    QCOMPARE(panel.stroke_width_slider()->value(), 7);
}

void BoardSessionStateQtTests::SceneGrowthLockPreventsRepeatedExpansionUntilTargetLeavesEdge() {
    BoardCanvasGrowthLocks locks;
    const auto initial_rect = initial_board_scene_rect();
    const QRectF near_right_edge(initial_rect.right() - 10.0, 0.0, 1.0, 1.0);

    const auto expanded_once = expand_scene_rect_for_target(initial_rect, near_right_edge, locks);
    QVERIFY(expanded_once.has_value());
    QVERIFY(expanded_once->right() > initial_rect.right());

    const QRectF still_near_new_right_edge(expanded_once->right() - 10.0, 0.0, 1.0, 1.0);
    const auto repeated = expand_scene_rect_for_target(*expanded_once, still_near_new_right_edge, locks);
    QVERIFY(!repeated.has_value());

    const QRectF away_from_right_edge(expanded_once->center(), QSizeF(1.0, 1.0));
    const auto away = expand_scene_rect_for_target(*expanded_once, away_from_right_edge, locks);
    QVERIFY(!away.has_value());

    const QRectF near_right_edge_again(expanded_once->right() - 10.0, 0.0, 1.0, 1.0);
    const auto expanded_again = expand_scene_rect_for_target(*expanded_once, near_right_edge_again, locks);
    QVERIFY(expanded_again.has_value());
    QVERIFY(expanded_again->right() > expanded_once->right());
}

}  // namespace online_board::client_qt

QTEST_MAIN(online_board::client_qt::BoardSessionStateQtTests)

#include "board_session_state_qt_tests.moc"
