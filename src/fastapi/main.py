from fastapi import FastAPI
from pydantic import BaseModel
from typing import List

app = FastAPI()

class AccelReading(BaseModel):
    x: float
    y: float
    z: float
    timestamp: str

@app.get("/")
async def read_root():
    return {"message": "Wassup bro!!!"}

@app.post("/data")
async def receive_data(readings: List[AccelReading]):
    print(f"Received {len(readings)} readings")
    for r in readings:
        print(r)
    return {"received": len(readings)}