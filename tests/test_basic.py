import pytest

# Start the tests via `make test-debug` or `make test-release`

async def test_basic(service_client):
    data = {"url": "http://example.com?id=123"}
    response = await service_client.post('/v1/make-shorter', json=data)
    assert response.status == 200
    
    response_json = response.json()
    assert 'short_url' in response_json

    request_url = response_json['short_url']
    request_url = request_url[request_url.rfind('/'):]

    response = await service_client.get(request_url)
    assert response.status == 200


async def test_bad_request(service_client):
    data = {}
    response = await service_client.post('/v1/make-shorter', json=data)
    assert response.status == 400


async def test_redirect_not_found(service_client):
    response = await service_client.get('/unknown-id')
    assert response.status == 404
