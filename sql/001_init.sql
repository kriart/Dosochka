ALTER USER dosochka WITH PASSWORD 'dosochka';
ALTER DATABASE dosochka OWNER TO dosochka;

CREATE TABLE IF NOT EXISTS users (
    id TEXT PRIMARY KEY,
    login TEXT NOT NULL UNIQUE,
    display_name TEXT NOT NULL,
    password_hash TEXT NOT NULL,
    created_at_ms BIGINT NOT NULL
);

CREATE TABLE IF NOT EXISTS auth_sessions (
    id TEXT PRIMARY KEY,
    user_id TEXT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    token_hash TEXT NOT NULL UNIQUE,
    created_at_ms BIGINT NOT NULL,
    expires_at_ms BIGINT NOT NULL
);

CREATE TABLE IF NOT EXISTS guest_sessions (
    id TEXT PRIMARY KEY,
    nickname TEXT NOT NULL,
    created_at_ms BIGINT NOT NULL
);

CREATE TABLE IF NOT EXISTS boards (
    id TEXT PRIMARY KEY,
    owner_user_id TEXT NOT NULL REFERENCES users(id) ON DELETE RESTRICT,
    title TEXT NOT NULL,
    access_mode TEXT NOT NULL,
    guest_policy TEXT NOT NULL,
    password_hash TEXT NULL,
    last_revision BIGINT NOT NULL,
    created_at_ms BIGINT NOT NULL,
    updated_at_ms BIGINT NOT NULL
);

CREATE TABLE IF NOT EXISTS board_members (
    board_id TEXT NOT NULL REFERENCES boards(id) ON DELETE CASCADE,
    user_id TEXT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    role TEXT NOT NULL,
    created_at_ms BIGINT NOT NULL,
    PRIMARY KEY (board_id, user_id)
);

CREATE TABLE IF NOT EXISTS board_objects (
    object_id TEXT NOT NULL,
    board_id TEXT NOT NULL REFERENCES boards(id) ON DELETE CASCADE,
    object_type TEXT NOT NULL,
    created_by_type TEXT NOT NULL,
    created_by_key TEXT NOT NULL,
    created_by_nickname TEXT NULL,
    created_at_ms BIGINT NOT NULL,
    updated_at_ms BIGINT NOT NULL,
    is_deleted BOOLEAN NOT NULL,
    z_index BIGINT NOT NULL,
    payload_json JSONB NOT NULL,
    PRIMARY KEY (board_id, object_id)
);

CREATE TABLE IF NOT EXISTS board_operations (
    operation_id TEXT PRIMARY KEY,
    board_id TEXT NOT NULL REFERENCES boards(id) ON DELETE CASCADE,
    actor_type TEXT NOT NULL,
    actor_key TEXT NOT NULL,
    actor_nickname TEXT NULL,
    revision BIGINT NOT NULL,
    applied_at_ms BIGINT NOT NULL,
    payload_json JSONB NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_auth_sessions_token_hash
    ON auth_sessions(token_hash);

CREATE INDEX IF NOT EXISTS idx_board_members_board_id
    ON board_members(board_id);

CREATE INDEX IF NOT EXISTS idx_board_objects_board_id
    ON board_objects(board_id);

CREATE INDEX IF NOT EXISTS idx_board_operations_board_revision
    ON board_operations(board_id, revision);
