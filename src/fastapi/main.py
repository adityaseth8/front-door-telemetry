from fastapi import FastAPI
from pydantic import BaseModel
from typing import List

app = FastAPI()

stored_data = []

class AccelReading(BaseModel):
    x: float
    y: float
    z: float
    timestamp: str

@app.get("/")
async def read_root():
    return {"message": "Wassup bro!!!", "data": stored_data}

@app.post("/data")
async def receive_data(readings: List[AccelReading]):
    print(f"Received {len(readings)} readings")
    global stored_data
    stored_data = readings
    return {"received": len(readings)}