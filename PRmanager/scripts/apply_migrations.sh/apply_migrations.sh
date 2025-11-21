#!/bin/bash
set -e

# Wait for PostgreSQL to be ready
until PGPASSWORD=${POSTGRES_PASSWORD} psql -h "${POSTGRES_HOST}" -U "${POSTGRES_USER}" -d "${POSTGRES_DB}" -c '\q'; do
  echo "Postgres is unavailable - sleeping"
  sleep 1
done

echo "PostgreSQL is up - applying migrations"

# Get migrations directory from first argument, default to /migrations
MIGRATIONS_DIR="${1:-/migrations}"

# Apply migrations in order
for migration_file in "$MIGRATIONS_DIR"/*.sql; do
    if [ -f "$migration_file" ]; then
        echo "Applying migration: $(basename $migration_file)"
        PGPASSWORD=${POSTGRES_PASSWORD} psql -h "${POSTGRES_HOST}" -U "${POSTGRES_USER}" -d "${POSTGRES_DB}" -f "$migration_file"
        echo "Migration applied: $(basename $migration_file)"
    fi
done

echo "All migrations applied successfully"
