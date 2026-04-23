#include "board_state_detail.hpp"

#include <utility>

namespace online_board::application::detail {

common::Result<domain::BoardSnapshot> fail_snapshot(common::ErrorCode code, std::string message) {
    return common::fail<domain::BoardSnapshot>(code, std::move(message));
}

bool has_positive_size(const domain::Size& size) {
    return size.width > 0.0 && size.height > 0.0;
}

bool has_positive_rect(const domain::Rect& rect) {
    return rect.width > 0.0 && rect.height > 0.0;
}

bool has_non_empty_text(const std::string& value) {
    return !value.empty();
}

domain::BoardObject* find_active_object(
    domain::BoardSnapshot& snapshot,
    const common::ObjectId& object_id) {
    auto it = snapshot.objects.find(object_id);
    if (it == snapshot.objects.end() || it->second.is_deleted) {
        return nullptr;
    }
    return &it->second;
}

common::Result<domain::BoardSnapshot> add_object(
    domain::BoardSnapshot& snapshot,
    domain::BoardObject object) {
    if (snapshot.objects.contains(object.object_id)) {
        return fail_snapshot(common::ErrorCode::already_exists, "Object already exists");
    }
    snapshot.objects.emplace(object.object_id, std::move(object));
    return snapshot;
}

}  // namespace online_board::application::detail
