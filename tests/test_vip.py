import pytest

# Start the tests via `make test-debug` or `make test-release`

async def test_vip_scenario(service_client):
    data = {"url": "https://example.com?id=vip", "vip_key": "example_vip"}
    response = await service_client.post('/v1/make-shorter', json=data)
    assert response.status_code == 200

    response_json = response.json()
    request_url = response_json['short_url']
    request_url = request_url[request_url.rfind('/'):]
    assert request_url == '/' + data['vip_key']
    
    response = await service_client.post('/v1/make-shorter', json=data)
    assert response.status_code == 400


async def test_time_to_live_maximum(service_client):
    data = {
        "url": "https://example.com?id=ttl_max",
        "vip_key": "example_ttl_max",
        "time_to_live": 49,  # HOURS - default unit
    }
    response = await service_client.post('/v1/make-shorter', json=data)
    assert response.status_code == 400

    data["time_to_live"] = 48
    response = await service_client.post('/v1/make-shorter', json=data)
    assert response.status_code == 200


async def test_time_to_live_expiration(service_client, mocked_time):
    data = {
        "url": "https://example.com?id=ttl_exp",
        "vip_key": "example_ttl_exp",
        "time_to_live": 10,
        "time_to_live_unit": "SECONDS",
    }


    response = await service_client.post('/v1/make-shorter', json=data)
    assert response.status_code == 200

    response_json = response.json()
    assert 'short_url' in response_json

    request_url = response_json['short_url']
    request_url = request_url[request_url.rfind('/'):]

    response = await service_client.get(request_url)
    assert response.status == 200

    mocked_time.sleep(60)

    response = await service_client.get(request_url)
    assert response.status == 404

    response = await service_client.post('/v1/make-shorter', json=data)
    assert response.status_code == 200

    response = await service_client.get(request_url)
    assert response.status == 200
