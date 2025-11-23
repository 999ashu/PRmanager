DROP SCHEMA IF EXISTS prmanager CASCADE;
CREATE SCHEMA IF NOT EXISTS prmanager;

CREATE TABLE prmanager.teams (
    name TEXT PRIMARY KEY
);

CREATE TABLE prmanager.users (
    id TEXT PRIMARY KEY,
    username TEXT NOT NULL,
    team_name TEXT NOT NULL REFERENCES prmanager.teams(name),
    is_active BOOLEAN NOT NULL DEFAULT TRUE
);

CREATE TABLE prmanager.pull_requests (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    author_id TEXT NOT NULL REFERENCES prmanager.users(id),
    status TEXT NOT NULL DEFAULT 'OPEN' CHECK (status IN ('OPEN', 'MERGED')),
    created_at TIMESTAMPTZ DEFAULT NOW(),
    merged_at TIMESTAMPTZ
);

CREATE TABLE prmanager.reviewers (
    pull_request_id TEXT NOT NULL REFERENCES prmanager.pull_requests(id),
    reviewer_id TEXT NOT NULL REFERENCES prmanager.users(id),
    PRIMARY KEY (pull_request_id, reviewer_id)
);
