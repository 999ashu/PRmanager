CREATE SCHEMA IF NOT EXISTS prmanager;

CREATE TABLE IF NOT EXISTS prmanager.teams (
    name TEXT PRIMARY KEY,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS prmanager.users (
    id TEXT PRIMARY KEY,
    username TEXT NOT NULL,
    team_name TEXT NOT NULL REFERENCES prmanager.teams(name) ON DELETE CASCADE,
    is_active BOOLEAN NOT NULL DEFAULT TRUE,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_users_team_name ON prmanager.users(team_name);
CREATE INDEX IF NOT EXISTS idx_users_is_active ON prmanager.users(is_active);

CREATE TABLE IF NOT EXISTS prmanager.pull_requests (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    author_id TEXT NOT NULL REFERENCES prmanager.users(id) ON DELETE CASCADE,
    status TEXT NOT NULL DEFAULT 'OPEN' CHECK (status IN ('OPEN', 'MERGED')),
    created_at TIMESTAMPTZ DEFAULT NOW(),
    merged_at TIMESTAMPTZ
);

CREATE INDEX IF NOT EXISTS idx_pr_author_id ON prmanager.pull_requests(author_id);
CREATE INDEX IF NOT EXISTS idx_pr_status ON prmanager.pull_requests(status);

CREATE TABLE IF NOT EXISTS prmanager.reviewers (
    pull_request_id TEXT NOT NULL REFERENCES prmanager.pull_requests(id) ON DELETE CASCADE,
    reviewer_id TEXT NOT NULL REFERENCES prmanager.users(id) ON DELETE CASCADE,
    assigned_at TIMESTAMPTZ DEFAULT NOW(),
    PRIMARY KEY (pull_request_id, reviewer_id)
);

CREATE INDEX IF NOT EXISTS idx_reviewers_reviewer_id ON prmanager.reviewers(reviewer_id);
