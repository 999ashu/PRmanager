import pytest


async def test_team_add_success(service_client):
    team_data = {
        "team_name": "backend",
        "members": [
            {"user_id": "u1", "username": "Alice", "is_active": True},
            {"user_id": "u2", "username": "Bob", "is_active": True},
        ],
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
        "members": [{"user_id": "u3", "username": "Charlie", "is_active": True}],
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
        ],
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


async def test_team_add_moves_user(service_client):
    # Add user to team A
    team_a = {"team_name": "A", "members": [
        {"user_id": "u_move", "username": "Mover", "is_active": True}]}
    resp = await service_client.post("/team/add", json=team_a)
    assert resp.status == 201

    # Add another team B with the same user id (should update user's team via ON CONFLICT)
    team_b = {"team_name": "B", "members": [
        {"user_id": "u_move", "username": "Mover", "is_active": True}]}
    resp = await service_client.post("/team/add", json=team_b)
    assert resp.status == 201

    # Verify user now belongs to team B
    resp = await service_client.get("/team/get", params={"team_name": "B"})
    assert resp.status == 200
    data = resp.json()
    ids = [m["user_id"] for m in data["members"]]
    assert "u_move" in ids
