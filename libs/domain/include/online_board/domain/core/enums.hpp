#pragma once

namespace online_board::domain {

enum class PrincipalType {
    registered_user,
    guest
};

enum class BoardRole {
    owner,
    editor,
    viewer
};

enum class AccessMode {
    private_board,
    password_protected
};

enum class GuestPolicy {
    guest_disabled,
    guest_view_only,
    guest_edit_allowed
};

enum class ObjectType {
    stroke,
    shape,
    text
};

enum class ShapeType {
    rectangle,
    ellipse,
    line
};

}  // namespace online_board::domain
