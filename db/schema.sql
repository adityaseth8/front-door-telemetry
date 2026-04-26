CREATE TABLE IF NOT EXISTS raw_readings (
    id          SERIAL PRIMARY KEY,
    x           FLOAT NOT NULL,
    y           FLOAT NOT NULL,
    z           FLOAT NOT NULL,
    timestamp   TIMESTAMPTZ NOT NULL,
    created_at  TIMESTAMPTZ DEFAULT NOW()
);