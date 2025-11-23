import pytest


async def test_user_set_is_active(service_client):
    team_data = {
        "team_name": "devops",
        "members": [{"user_id": "u20", "username": "Frank", "is_active": True}],
    }
    await service_client.post("/team/add", json=team_data)

    response = await service_client.post(
        "/users/setIsActive", json={"user_id": "u20", "is_active": False}
    )
    assert response.status == 200
    data = response.json()
    assert data["user"]["user_id"] == "u20"
    assert data["user"]["is_active"] is False


async def test_user_set_is_active_not_found(service_client):
    response = await service_client.post(
        "/users/setIsActive", json={"user_id": "nonexistent", "is_active": False}
    )
    assert response.status == 404
    data = response.json()
    assert data["error"]["code"] == "NOT_FOUND"


async def test_user_get_review_missing_param(service_client):
    # If user_id is missing the handler raises a client error; ensure non-200
    response = await service_client.get("/users/getReview")
    assert response.status >= 400


async def test_mass_deactivate_zero(service_client):
    # Call massDeactivate with non-existing ids -> zero deactivated
    response = await service_client.post("/users/massDeactivate", json={"user_ids": ["no1", "no2"]})
    assert response.status == 200
    data = response.json()
    assert data["deactivated_count"] == 0
