from datetime import datetime
from dotenv import load_dotenv
from fastapi import FastAPI
from pydantic import BaseModel
from typing import List
import asyncpg
import os

app = FastAPI()
load_dotenv()

DB_URL = (
    f"postgresql://{os.getenv('POSTGRES_USER')}:{os.getenv('POSTGRES_PASSWORD')}"
    f"@{os.getenv('POSTGRES_HOST')}/{os.getenv('POSTGRES_DB')}"
)

async def get_db():
    return await asyncpg.connect(DB_URL)

stored_data = []

class AccelReading(BaseModel):
    x: float
    y: float
    z: float
    timestamp: datetime

@app.get("/")
async def read_root():
    return {"message": "Wassup bro!!!", "data": stored_data}

@app.post("/data")
async def receive_data(readings: List[AccelReading]):
    print(f"Received {len(readings)} readings")
    conn = await get_db()
    try:
        await conn.executemany(
            "INSERT INTO raw_readings (x, y, z, timestamp) VALUES ($1, $2, $3, $4)",
            [(r.x, r.y, r.z, r.timestamp) for r in readings]
        )
    finally:
        await conn.close()
    return {"inserted": len(readings)}
    # global stored_data
    # stored_data = readings
    # return {"received": len(readings)}

@app.get("/readings")
async def get_readings(limit: int = 100):
    conn = await get_db()
    try:
        rows = await conn.fetch(
            "SELECT * FROM raw_readings ORDER BY timestamp DESC LIMIT $1", limit
        )
        return [dict(r) for r in rows]
    finally:
        await conn.close()