import pytest

from testsuite.databases.pgsql import discover

pytest_plugins = [
    "pytest_userver.plugins.core",
    "pytest_userver.plugins.postgresql",
]


@pytest.fixture(scope="session")
def initial_data_path(service_source_dir):
    """Path for find files with data"""
    return [
        service_source_dir / "postgresql/data",
    ]


@pytest.fixture(scope="session")
def pgsql_local(service_source_dir, pgsql_local_create):
    """Create schemas databases for tests"""
    databases = discover.find_schemas(
        "PRmanager",
        [service_source_dir.joinpath("postgresql/schemas")],
    )
    return pgsql_local_create(list(databases.values()))


@pytest.fixture
def create_team(service_client):
    """Return a helper to create teams during tests."""

    async def _create(team_data):
        resp = await service_client.post("/team/add", json=team_data)
        return resp

    return _create


@pytest.fixture
def create_pr(service_client):
    """Return a helper to create pull requests during tests."""

    async def _create(pr_data):
        resp = await service_client.post("/pullRequest/create", json=pr_data)
        return resp

    return _create
