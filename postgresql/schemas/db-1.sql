CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

DROP SCHEMA IF EXISTS url_shortener CASCADE;

CREATE SCHEMA IF NOT EXISTS url_shortener;

CREATE TABLE IF NOT EXISTS url_shortener.urls (
    id TEXT PRIMARY KEY DEFAULT uuid_generate_v4(),
    url TEXT NOT NULL,
    -- short_url TEXT UNIQUE,
    expiration_time TIMESTAMP
);
