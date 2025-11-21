# Integration tests for PR Manager Service

import pytest


async def test_team_add_success(service_client):
    team_data = {
        "team_name": "backend",
        "members": [
            {"user_id": "u1", "username": "Alice", "is_active": True},
            {"user_id": "u2", "username": "Bob", "is_active": True},
        ]
    }
    
    response = await service_client.post("/team/add", json=team_data)
    assert response.status == 201
    data = response.json()
    assert "team" in data
    assert data["team"]["team_name"] == "backend"
    assert len(data["team"]["members"]) == 2


async def test_team_add_duplicate(service_client):
    team_data = {
        "team_name": "payments",
        "members": [
            {"user_id": "u3", "username": "Charlie", "is_active": True},
        ]
    }
    
    response = await service_client.post("/team/add", json=team_data)
    assert response.status == 201
    
    response = await service_client.post("/team/add", json=team_data)
    assert response.status == 400
    data = response.json()
    assert data["error"]["code"] == "TEAM_EXISTS"


async def test_team_get_success(service_client):
    team_data = {
        "team_name": "frontend",
        "members": [
            {"user_id": "u10", "username": "Dave", "is_active": True},
            {"user_id": "u11", "username": "Eve", "is_active": False},
        ]
    }
    await service_client.post("/team/add", json=team_data)
    
    response = await service_client.get("/team/get", params={"team_name": "frontend"})
    assert response.status == 200
    data = response.json()
    assert data["team_name"] == "frontend"
    assert len(data["members"]) == 2


async def test_team_get_not_found(service_client):
    response = await service_client.get("/team/get", params={"team_name": "nonexistent"})
    assert response.status == 404
    data = response.json()
    assert data["error"]["code"] == "NOT_FOUND"


async def test_user_set_is_active(service_client):
    team_data = {
        "team_name": "devops",
        "members": [
            {"user_id": "u20", "username": "Frank", "is_active": True},
        ]
    }
    await service_client.post("/team/add", json=team_data)
    
    response = await service_client.post(
        "/users/setIsActive",
        json={"user_id": "u20", "is_active": False}
    )
    assert response.status == 200
    data = response.json()
    assert data["user"]["user_id"] == "u20"
    assert data["user"]["is_active"] is False


async def test_user_set_is_active_not_found(service_client):
    response = await service_client.post(
        "/users/setIsActive",
        json={"user_id": "nonexistent", "is_active": False}
    )
    assert response.status == 404
    data = response.json()
    assert data["error"]["code"] == "NOT_FOUND"


async def test_pr_create_with_reviewers(service_client):
    team_data = {
        "team_name": "mobile",
        "members": [
            {"user_id": "u30", "username": "Grace", "is_active": True},
            {"user_id": "u31", "username": "Henry", "is_active": True},
            {"user_id": "u32", "username": "Ivy", "is_active": True},
        ]
    }
    await service_client.post("/team/add", json=team_data)
    
    pr_data = {
        "pull_request_id": "pr-100",
        "pull_request_name": "Add mobile feature",
        "author_id": "u30"
    }
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
        ]
    }
    await service_client.post("/team/add", json=team_data)
    
    pr_data = {
        "pull_request_id": "pr-101",
        "pull_request_name": "Fix bug",
        "author_id": "u40"
    }
    response = await service_client.post("/pullRequest/create", json=pr_data)
    assert response.status == 201
    data = response.json()
    assert len(data["pr"]["assigned_reviewers"]) == 0


async def test_pr_create_duplicate(service_client):
    team_data = {
        "team_name": "test_team",
        "members": [
            {"user_id": "u50", "username": "Leo", "is_active": True},
        ]
    }
    await service_client.post("/team/add", json=team_data)
    
    pr_data = {
        "pull_request_id": "pr-102",
        "pull_request_name": "Test PR",
        "author_id": "u50"
    }
    
    response = await service_client.post("/pullRequest/create", json=pr_data)
    assert response.status == 201
    
    response = await service_client.post("/pullRequest/create", json=pr_data)
    assert response.status == 409
    data = response.json()
    assert data["error"]["code"] == "PR_EXISTS"


async def test_pr_merge_success(service_client):
    team_data = {
        "team_name": "api",
        "members": [
            {"user_id": "u60", "username": "Mike", "is_active": True},
            {"user_id": "u61", "username": "Nina", "is_active": True},
        ]
    }
    await service_client.post("/team/add", json=team_data)
    
    pr_data = {
        "pull_request_id": "pr-103",
        "pull_request_name": "API update",
        "author_id": "u60"
    }
    await service_client.post("/pullRequest/create", json=pr_data)
    
    response = await service_client.post(
        "/pullRequest/merge",
        json={"pull_request_id": "pr-103"}
    )
    assert response.status == 200
    data = response.json()
    assert data["pr"]["status"] == "MERGED"
    assert "mergedAt" in data["pr"]


async def test_pr_merge_idempotent(service_client):
    team_data = {
        "team_name": "security",
        "members": [
            {"user_id": "u70", "username": "Oscar", "is_active": True},
        ]
    }
    await service_client.post("/team/add", json=team_data)
    
    pr_data = {
        "pull_request_id": "pr-104",
        "pull_request_name": "Security fix",
        "author_id": "u70"
    }
    await service_client.post("/pullRequest/create", json=pr_data)
    
    response1 = await service_client.post(
        "/pullRequest/merge",
        json={"pull_request_id": "pr-104"}
    )
    assert response1.status == 200
    merged_at_1 = response1.json()["pr"]["mergedAt"]
    
    response2 = await service_client.post(
        "/pullRequest/merge",
        json={"pull_request_id": "pr-104"}
    )
    assert response2.status == 200
    merged_at_2 = response2.json()["pr"]["mergedAt"]
    
    assert merged_at_1 == merged_at_2


async def test_pr_reassign_success(service_client):
    team_data = {
        "team_name": "data",
        "members": [
            {"user_id": "u80", "username": "Paul", "is_active": True},
            {"user_id": "u81", "username": "Quinn", "is_active": True},
            {"user_id": "u82", "username": "Rachel", "is_active": True},
            {"user_id": "u83", "username": "Steve", "is_active": True},
        ]
    }
    await service_client.post("/team/add", json=team_data)
    
    pr_data = {
        "pull_request_id": "pr-105",
        "pull_request_name": "Data pipeline",
        "author_id": "u80"
    }
    create_response = await service_client.post("/pullRequest/create", json=pr_data)
    reviewers = create_response.json()["pr"]["assigned_reviewers"]
    assert len(reviewers) == 2
    
    old_reviewer = reviewers[0]
    response = await service_client.post(
        "/pullRequest/reassign",
        json={"pull_request_id": "pr-105", "old_user_id": old_reviewer}
    )
    assert response.status == 200
    data = response.json()
    assert "replaced_by" in data
    assert data["replaced_by"] not in reviewers
    assert old_reviewer not in data["pr"]["assigned_reviewers"]


async def test_pr_reassign_merged_pr(service_client):
    team_data = {
        "team_name": "cloud",
        "members": [
            {"user_id": "u90", "username": "Sam", "is_active": True},
            {"user_id": "u91", "username": "Tina", "is_active": True},
        ]
    }
    await service_client.post("/team/add", json=team_data)
    
    pr_data = {
        "pull_request_id": "pr-106",
        "pull_request_name": "Cloud migration",
        "author_id": "u90"
    }
    create_response = await service_client.post("/pullRequest/create", json=pr_data)
    reviewers = create_response.json()["pr"]["assigned_reviewers"]
    
    await service_client.post(
        "/pullRequest/merge",
        json={"pull_request_id": "pr-106"}
    )
    
    if reviewers:
        response = await service_client.post(
            "/pullRequest/reassign",
            json={"pull_request_id": "pr-106", "old_user_id": reviewers[0]}
        )
        assert response.status == 409
        data = response.json()
        assert data["error"]["code"] == "PR_MERGED"


async def test_pr_reassign_not_assigned(service_client):
    team_data = {
        "team_name": "infra",
        "members": [
            {"user_id": "u100", "username": "Uma", "is_active": True},
            {"user_id": "u101", "username": "Victor", "is_active": True},
        ]
    }
    await service_client.post("/team/add", json=team_data)
    
    pr_data = {
        "pull_request_id": "pr-107",
        "pull_request_name": "Infrastructure update",
        "author_id": "u100"
    }
    await service_client.post("/pullRequest/create", json=pr_data)
    
    response = await service_client.post(
        "/pullRequest/reassign",
        json={"pull_request_id": "pr-107", "old_user_id": "u100"}
    )
    assert response.status == 409
    data = response.json()
    assert data["error"]["code"] == "NOT_ASSIGNED"


async def test_user_get_review(service_client):
    team_data = {
        "team_name": "platform",
        "members": [
            {"user_id": "u110", "username": "Wendy", "is_active": True},
            {"user_id": "u111", "username": "Xavier", "is_active": True},
            {"user_id": "u112", "username": "Yara", "is_active": True},
        ]
    }
    await service_client.post("/team/add", json=team_data)
    
    for i in range(3):
        pr_data = {
            "pull_request_id": f"pr-20{i}",
            "pull_request_name": f"Feature {i}",
            "author_id": "u110"
        }
        await service_client.post("/pullRequest/create", json=pr_data)
    
    response = await service_client.get("/users/getReview", params={"user_id": "u111"})
    assert response.status == 200
    data = response.json()
    assert data["user_id"] == "u111"
    assert "pull_requests" in data
    assert isinstance(data["pull_requests"], list)


async def test_ping_endpoint(service_client):
    response = await service_client.get("/ping")
    assert response.status == 200


async def test_mass_deactivate(service_client):
    team_data = {
        "team_name": "analytics",
        "members": [
            {"user_id": "u120", "username": "Zoe", "is_active": True},
            {"user_id": "u121", "username": "Adam", "is_active": True},
            {"user_id": "u122", "username": "Ben", "is_active": True},
            {"user_id": "u123", "username": "Carl", "is_active": True},
        ]
    }
    await service_client.post("/team/add", json=team_data)

    pr_data = {
        "pull_request_id": "pr-300",
        "pull_request_name": "Analytics dashboard",
        "author_id": "u120"
    }
    await service_client.post("/pullRequest/create", json=pr_data)

    response = await service_client.post(
        "/users/massDeactivate",
        json={"user_ids": ["u121", "u122"]}
    )
    assert response.status == 200
    data = response.json()
    assert data["deactivated_count"] == 2

    review_response = await service_client.get("/users/getReview", params={"user_id": "u121"})
    assert review_response.status == 200
    review_data = review_response.json()
    assert len(review_data["pull_requests"]) == 0


async def test_stats(service_client):
    response = await service_client.get("/stats")
    assert response.status == 200
    data = response.json()
    assert "teams_count" in data
    assert "users_count" in data
    assert "prs_count" in data
    assert isinstance(data["teams_count"], int)
    assert isinstance(data["users_count"], int)
    assert isinstance(data["prs_count"], int)
