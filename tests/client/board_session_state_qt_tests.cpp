#include <QtTest/QtTest>

#include "board_canvas_widget.hpp"
#include "board_session_state.hpp"

namespace online_board::client_qt {

class BoardSessionStateQtTests final : public QObject {
    Q_OBJECT

private slots:
    void DifferentClientStatesGenerateDistinctObjectIdsOnSameBoard();
    void EraserHitTestReturnsTopmostObject();
    void EraserDragRequestsDeletionForEachTouchedObject();
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

}  // namespace online_board::client_qt

QTEST_MAIN(online_board::client_qt::BoardSessionStateQtTests)

#include "board_session_state_qt_tests.moc"
