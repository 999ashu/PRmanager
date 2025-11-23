async def test_user_get_review(service_client):
    team_data = {
        "team_name": "platform",
        "members": [
            {"user_id": "u110", "username": "Wendy", "is_active": True},
            {"user_id": "u111", "username": "Xavier", "is_active": True},
            {"user_id": "u112", "username": "Yara", "is_active": True},
        ],
    }
    await service_client.post("/team/add", json=team_data)

    for i in range(3):
        pr_data = {"pull_request_id": f"pr-20{i}",
                   "pull_request_name": f"Feature {i}", "author_id": "u110"}
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
