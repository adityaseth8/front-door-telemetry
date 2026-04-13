# front-door-telemetry

Sometimes I leave home and can’t remember whether the front door was actually locked, and there’s no easy way to confirm it remotely. This project aims to solve that issue. 

Plan is to use an ADXL345 accelerometer mounted on the door and lock to capture motion and lock‑state patterns in real time. The sensor will stream live acceleration data into a SQL Server database, and a custom dashboard or app will display door status, open/close events, and historical analytics for better home awareness and security.
