#!/bin/bash
set -e

until PGPASSWORD=${POSTGRES_PASSWORD} psql -h "${POSTGRES_HOST}" -U "${POSTGRES_USER}" -d "${POSTGRES_DB}" -c '\q'; do
  echo "Postgres is unavailable - sleeping"
  sleep 1
done

echo "PostgreSQL is up - applying migrations"

MIGRATIONS_DIR="${1:-/migrations}"

for migration_file in "$MIGRATIONS_DIR"/*.sql; do
    if [ -f "$migration_file" ]; then
        echo "Applying migration: $(basename $migration_file)"
        PGPASSWORD=${POSTGRES_PASSWORD} psql -h "${POSTGRES_HOST}" -U "${POSTGRES_USER}" -d "${POSTGRES_DB}" -f "$migration_file"
        echo "Migration applied: $(basename $migration_file)"
    fi
done

echo "All migrations applied successfully"