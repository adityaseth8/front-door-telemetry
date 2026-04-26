Virtual Environment

in git bash terminal:
python -m venv env  | create virtual environment called env

\front-door-telemetry/src
idf.py build - in IDF powershell

check port that ESP32 shows up on in laptop
idf.py -p COM4 flash monitor

to activate
. env/Scripts/activate

// gcc write_to_fast_api.c -I"C:\msys64\ucrt64\include" -L"C:\msys64\ucrt64\lib" -lcurl -o your_program.exe

uvicorn main:app --reload

---

# Project Learnings & Setup Notes

## 1. `write_to_fast_api.c` (Test Data Generator)

This C file generates fake sensor data (x, y, z, timestamp) and sends it in batches to the FastAPI server. It simulates accelerometer readings for testing the backend pipeline.

**Key points:**
- Uses `libcurl` to POST JSON data to the FastAPI endpoint (`/data`).
- Batches readings for efficiency (see `BATCH_SIZE`).
- Generates timestamps with microsecond precision.

### Compiling on Windows (MSYS2/MinGW64)
- **Why `-I` and `-L`?**
	- `-I"C:\msys64\ucrt64\include"` tells GCC where to find header files (like `curl.h`).
	- `-L"C:\msys64\ucrt64\lib"` tells GCC where to find the required `.dll`/`.a` libraries (like `libcurl.dll.a`).
- **Why MSYS2/MinGW64?**
	- MSYS2 is a Unix-like environment for Windows, providing a package manager and build tools (like GCC, make, and libraries such as curl).
	- MinGW64 (within MSYS2) provides native Windows GCC toolchains and libraries.
- **How to get the DLLs:**
	- Use MSYS2's `pacman` to install `mingw-w64-ucrt-x86_64-curl` and dependencies. This puts the DLLs and headers in the right place.
	- Example: `pacman -S mingw-w64-ucrt-x86_64-curl`
- **Alternative:**
	- You can use a Makefile and build in WSL (Linux subsystem) if you prefer a Linux-like environment.

## 2. FastAPI Server & Uvicorn

- The FastAPI app (see `src/fastapi/main.py`) exposes endpoints for receiving and storing sensor data.
- To run the server (from the `src/fastapi` directory):
	- `uvicorn main:app --reload`
		- `main:app` refers to the `app` object in `main.py`.
		- `--reload` auto-restarts the server on code changes (useful for development).
- The `/data` endpoint accepts POSTed JSON batches and writes them to the database.

## 3. PostgreSQL Setup with Docker Compose

- The backend uses PostgreSQL for storing sensor data.
- **Why Docker Compose?**
	- Simplifies running a database locally without manual installation/configuration.
	- Ensures consistent environment across machines.
- **How it works:**
	- `docker-compose.yml` defines a `db` service using the official `postgres:16` image.
	- Environment variables (user, password, db name) are loaded from `.env`.
	- The database is exposed on port 5432.
	- The schema is initialized from `db/schema.sql` (mounted into the container).
- **To start Postgres:**
	- Run `docker compose up db` from the project root.
	- The FastAPI app connects using credentials from the same `.env` file.

## 4. Python Virtual Environment: Required Packages

After activating your virtual environment, install the following packages:

```sh
pip install fastapi uvicorn[standard] asyncpg python-dotenv pydantic
```

**Explanation:**
- `fastapi` — Web framework for the API server
- `uvicorn[standard]` — ASGI server to run FastAPI (with extra dependencies for production)
- `asyncpg` — Async PostgreSQL driver
- `python-dotenv` — Loads environment variables from `.env`
- `pydantic` — Data validation and settings management (FastAPI depends on this, but explicit install is safest)

You may also want:
- `httpx` or `requests` — For testing API endpoints from Python scripts

---
