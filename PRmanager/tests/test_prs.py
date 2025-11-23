import pytest


async def test_pr_create_with_reviewers(service_client):
    team_data = {
        "team_name": "mobile",
        "members": [
            {"user_id": "u30", "username": "Grace", "is_active": True},
            {"user_id": "u31", "username": "Henry", "is_active": True},
            {"user_id": "u32", "username": "Ivy", "is_active": True},
        ],
    }
    await service_client.post("/team/add", json=team_data)

    pr_data = {"pull_request_id": "pr-100",
               "pull_request_name": "Add mobile feature", "author_id": "u30"}
    response = await service_client.post("/pullRequest/create", json=pr_data)
    assert response.status == 201
    data = response.json()
    assert data["pr"]["pull_request_id"] == "pr-100"
    assert data["pr"]["status"] == "OPEN"
    assert len(data["pr"]["assigned_reviewers"]) == 2
    assert "u30" not in data["pr"]["assigned_reviewers"]


async def test_pr_create_with_inactive_users(service_client):
    team_data = {
        "team_name": "qa",
        "members": [
            {"user_id": "u40", "username": "Jack", "is_active": True},
            {"user_id": "u41", "username": "Kate", "is_active": False},
        ],
    }
    await service_client.post("/team/add", json=team_data)

    pr_data = {"pull_request_id": "pr-101",
               "pull_request_name": "Fix bug", "author_id": "u40"}
    response = await service_client.post("/pullRequest/create", json=pr_data)
    assert response.status == 201
    data = response.json()
    assert len(data["pr"]["assigned_reviewers"]) == 0


async def test_pr_create_duplicate(service_client):
    team_data = {"team_name": "test_team", "members": [
        {"user_id": "u50", "username": "Leo", "is_active": True}]}
    await service_client.post("/team/add", json=team_data)

    pr_data = {"pull_request_id": "pr-102",
               "pull_request_name": "Test PR", "author_id": "u50"}

    response = await service_client.post("/pullRequest/create", json=pr_data)
    assert response.status == 201

    response = await service_client.post("/pullRequest/create", json=pr_data)
    assert response.status == 409
    data = response.json()
    assert data["error"]["code"] == "PR_EXISTS"


async def test_pr_create_author_not_found(service_client):
    pr_data = {"pull_request_id": "pr-no-author",
               "pull_request_name": "No author", "author_id": "no-one"}
    response = await service_client.post("/pullRequest/create", json=pr_data)
    assert response.status == 404
    data = response.json()
    assert data["error"]["code"] == "NOT_FOUND"


async def test_pr_create_exact_two_candidates(service_client):
    team_data = {
        "team_name": "pair-team",
        "members": [
            {"user_id": "pa1", "username": "A", "is_active": True},
            {"user_id": "pa2", "username": "B", "is_active": True},
            {"user_id": "pa3", "username": "C", "is_active": True},
        ],
    }
    await service_client.post("/team/add", json=team_data)
    pr_data = {"pull_request_id": "pr-pair",
               "pull_request_name": "Pair", "author_id": "pa1"}
    resp = await service_client.post("/pullRequest/create", json=pr_data)
    assert resp.status == 201
    data = resp.json()
    # there are two candidates (pa2, pa3) -> both should be assigned
    assert len(data["pr"]["assigned_reviewers"]) == 2


async def test_pr_merge_success_and_not_found(service_client):
    team_data = {"team_name": "api", "members": [{"user_id": "u60", "username": "Mike", "is_active": True}, {
        "user_id": "u61", "username": "Nina", "is_active": True}]}
    await service_client.post("/team/add", json=team_data)
    pr_data = {"pull_request_id": "pr-103",
               "pull_request_name": "API update", "author_id": "u60"}
    await service_client.post("/pullRequest/create", json=pr_data)

    response = await service_client.post("/pullRequest/merge", json={"pull_request_id": "pr-103"})
    assert response.status == 200
    data = response.json()
    assert data["pr"]["status"] == "MERGED"
    assert "mergedAt" in data["pr"]

    # Merge non-existent PR
    response = await service_client.post("/pullRequest/merge", json={"pull_request_id": "no-such-pr"})
    assert response.status == 404
    data = response.json()
    assert data["error"]["code"] == "NOT_FOUND"


async def test_pr_merge_idempotent(service_client):
    team_data = {"team_name": "security", "members": [
        {"user_id": "u70", "username": "Oscar", "is_active": True}]}
    await service_client.post("/team/add", json=team_data)
    pr_data = {"pull_request_id": "pr-104",
               "pull_request_name": "Security fix", "author_id": "u70"}
    await service_client.post("/pullRequest/create", json=pr_data)

    response1 = await service_client.post("/pullRequest/merge", json={"pull_request_id": "pr-104"})
    assert response1.status == 200
    merged_at_1 = response1.json()["pr"]["mergedAt"]

    response2 = await service_client.post("/pullRequest/merge", json={"pull_request_id": "pr-104"})
    assert response2.status == 200
    merged_at_2 = response2.json()["pr"]["mergedAt"]

    assert merged_at_1 == merged_at_2


async def test_pr_reassign_variants(service_client):
    # success case
    team_data = {
        "team_name": "data",
        "members": [
            {"user_id": "u80", "username": "Paul", "is_active": True},
            {"user_id": "u81", "username": "Quinn", "is_active": True},
            {"user_id": "u82", "username": "Rachel", "is_active": True},
            {"user_id": "u83", "username": "Steve", "is_active": True},
        ],
    }
    await service_client.post("/team/add", json=team_data)

    pr_data = {"pull_request_id": "pr-105",
               "pull_request_name": "Data pipeline", "author_id": "u80"}
    create_response = await service_client.post("/pullRequest/create", json=pr_data)
    reviewers = create_response.json()["pr"]["assigned_reviewers"]
    assert len(reviewers) == 2

    old_reviewer = reviewers[0]
    response = await service_client.post("/pullRequest/reassign", json={"pull_request_id": "pr-105", "old_user_id": old_reviewer})
    assert response.status == 200
    data = response.json()
    assert "replaced_by" in data
    assert data["replaced_by"] not in reviewers
    assert old_reviewer not in data["pr"]["assigned_reviewers"]

    # merged PR -> cannot reassign
    team_data = {"team_name": "cloud", "members": [{"user_id": "u90", "username": "Sam", "is_active": True}, {
        "user_id": "u91", "username": "Tina", "is_active": True}]}
    await service_client.post("/team/add", json=team_data)
    pr_data = {"pull_request_id": "pr-106",
               "pull_request_name": "Cloud migration", "author_id": "u90"}
    create_response = await service_client.post("/pullRequest/create", json=pr_data)
    reviewers = create_response.json()["pr"]["assigned_reviewers"]
    await service_client.post("/pullRequest/merge", json={"pull_request_id": "pr-106"})
    if reviewers:
        response = await service_client.post("/pullRequest/reassign", json={"pull_request_id": "pr-106", "old_user_id": reviewers[0]})
        assert response.status == 409
        data = response.json()
        assert data["error"]["code"] == "PR_MERGED"

    # not assigned -> error
    team_data = {"team_name": "infra", "members": [{"user_id": "u100", "username": "Uma", "is_active": True}, {
        "user_id": "u101", "username": "Victor", "is_active": True}]}
    await service_client.post("/team/add", json=team_data)
    pr_data = {"pull_request_id": "pr-107",
               "pull_request_name": "Infrastructure update", "author_id": "u100"}
    await service_client.post("/pullRequest/create", json=pr_data)
    response = await service_client.post("/pullRequest/reassign", json={"pull_request_id": "pr-107", "old_user_id": "u100"})
    assert response.status == 409
    data = response.json()
    assert data["error"]["code"] == "NOT_ASSIGNED"


async def test_pr_reassign_no_candidate(service_client):
    # Create team where all other team members are already reviewers -> NO_CANDIDATE
    team_data = {
        "team_name": "nocand",
        "members": [
            {"user_id": "na1", "username": "A", "is_active": True},
            {"user_id": "na2", "username": "B", "is_active": True},
            {"user_id": "na3", "username": "C", "is_active": True},
        ],
    }
    await service_client.post("/team/add", json=team_data)
    pr_data = {"pull_request_id": "pr-nocand",
               "pull_request_name": "NoCand", "author_id": "na1"}
    resp = await service_client.post("/pullRequest/create", json=pr_data)
    assert resp.status == 201
    reviewers = resp.json()["pr"]["assigned_reviewers"]
    # reviewers should include na2 and na3; now try to reassign na2 -> no candidate
    if reviewers:
        response = await service_client.post("/pullRequest/reassign", json={"pull_request_id": "pr-nocand", "old_user_id": reviewers[0]})
        assert response.status == 409
        data = response.json()
        assert data["error"]["code"] == "NO_CANDIDATE"
