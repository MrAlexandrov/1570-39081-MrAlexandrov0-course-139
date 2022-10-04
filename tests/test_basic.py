import pytest

from testsuite.databases import pgsql


# Start the tests via `make test-debug` or `make test-release`


async def test_basic(service_client):
    response = await service_client.post('/v1/make-shorter', data='{"url": "http://example.com?id=123"}')
    assert response.status == 200
    
    response_json = response.json()
    assert 'short_url' in response_json

    request_uri = response_json['short_url']
    request_uri = request_uri[request_uri.rfind('/'):]

    response = await service_client.get(request_uri)
    assert response.status == 200
