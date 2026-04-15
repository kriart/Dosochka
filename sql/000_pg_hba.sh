#!/bin/sh
set -eu

if ! grep -q '^host all all all trust$' "$PGDATA/pg_hba.conf"; then
  printf '\nhost all all all trust\n' >> "$PGDATA/pg_hba.conf"
fi

psql -v ON_ERROR_STOP=1 --username "$POSTGRES_USER" --dbname "$POSTGRES_DB" -c "SELECT pg_reload_conf();"
