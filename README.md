# 🅿️ Multi-Level Parking Garage Management System

A full-stack parking garage simulator with a **C backend** (all data structures hand-rolled — no STL), a **glassmorphism web frontend** (HTML/CSS/JS), and a **Python Flask bridge** connecting them.

> **60 slots · 3 floors · 5 data structures · 4 color themes · accelerated time · AED billing**

---

## Architecture

```mermaid
flowchart TB
    A["🌐 Browser\n(HTML / CSS / JS)"]
    B["🐍 Flask Server\n(server.py · port 5000)"]
    C["⚙️ C Executable\n(./parking)"]
    D["💾 garage.dat\n(binary state file)"]

    A -- "HTTP / JSON request" --> B
    B -- "subprocess call" --> C
    C -- "JSON stdout" --> B
    B -- "JSON response" --> A
    C -- "fwrite (save)" --> D
    D -- "fread (load)" --> C
```

**Why this design?**

- The C code stays **pure** — zero networking, zero HTTP parsing. Just data structures and algorithms.
- Flask acts as a **thin bridge** (~100 lines): receives HTTP requests, calls the C executable via `subprocess`, and returns its JSON stdout.
- State persists in a **binary file** (`garage.dat`) so every subprocess call is stateless, but the garage remembers everything between requests.

---

## Features

- **4 Color Themes** — Aurora (dark blue), Ember (warm red), Frost (light glass), Sorbet (pastel pink). Saved to localStorage.
- **Terminal Boot Sequence** — animated "system init" overlay on first load.
- **Seed Data** — garage starts pre-populated with 12 vehicles (4 VIP) and 2 in the exit lane.
- **Random Plate Generation** — click any action with an empty input and a random plate is auto-generated.
- **Accelerated Time** — 10 real seconds = 1 garage hour. Live garage clock in the pricing panel.
- **AED Billing** — 10 AED/hour flat rate, 15-minute grace period, 20% VIP discount, 200 AED daily cap.
- **Responsive Glassmorphism** — frosted glass panels, backdrop blur, gradient backgrounds.

---

## Data Structures

| # | Structure | Source Files | Purpose | Key Operations |
|---|-----------|-------------|---------|----------------|
| 1 | **Circular Queue** | `circular_queue.c/h` | Tracks available slot indices per floor. Dequeue = park, enqueue = free. Wraps around for efficient reuse. | `enqueue` · `dequeue` — O(1) |
| 2 | **Min-Heap Priority Queue** | `priority_queue.c/h` | VIP-prioritised waitlist. When full, vehicles queue by priority (VIP > Reserved > Regular) with FIFO tie-breaking. | `insert` · `extract_min` — O(log n) |
| 3 | **Stack** (Linked List) | `stack.c/h` | Exit-lane LIFO simulation. Models a single-lane exit where only the last car in can leave first. | `push` · `pop` · `peek` — O(1) |
| 4 | **Singly Linked List** | `linked_list.c/h` | Chronological activity log. Every park/exit event is appended at the tail. | `append` — O(1) |
| 5 | **Hash Table** (DJB2 + Chaining) | `hash_table.c/h` | License plate → parking location lookup. DJB2 hash with 101 buckets and separate chaining. | `insert` · `lookup` · `delete` — O(1) avg |

---

## Project Structure

```
parking-garage/
├── backend/
│   ├── include/              # Header files (8)
│   │   ├── circular_queue.h
│   │   ├── priority_queue.h
│   │   ├── stack.h
│   │   ├── linked_list.h
│   │   ├── hash_table.h
│   │   ├── garage.h          # Core orchestration layer
│   │   ├── billing.h         # TIME_SCALE, AED rates
│   │   └── json_helpers.h
│   ├── src/                  # Implementation files (9)
│   │   ├── circular_queue.c
│   │   ├── priority_queue.c
│   │   ├── stack.c
│   │   ├── linked_list.c
│   │   ├── hash_table.c
│   │   ├── garage.c          # Ties all 5 structures together
│   │   ├── billing.c         # 10 AED/hr, time-scaled billing
│   │   ├── json_helpers.c    # Zero-dependency JSON builder
│   │   └── main.c            # CLI dispatcher (11 commands)
│   ├── tests/
│   │   └── test_all.c        # Unit tests
│   └── Makefile
├── frontend/
│   ├── index.html            # Dashboard layout + boot overlay
│   ├── css/style.css         # 4-theme glassmorphism design system
│   └── js/
│       ├── api.js            # Fetch-based API client
│       ├── components.js     # Pure UI renderers
│       └── app.js            # Controller, themes, seed, clock
├── server.py                 # Flask subprocess bridge
├── requirements.txt          # flask
├── LICENSE                   # MIT
└── .gitignore
```

---

## Quick Start

### Prerequisites

- **GCC** (or any C compiler)
- **Python 3** + **pip**

### 1. Build the Backend

```bash
cd backend
make
```

### 2. Run Unit Tests

```bash
make test
```

### 3. Install Flask & Start Server

```bash
cd ..
pip3 install -r requirements.txt
python3 server.py
```

### 4. Open the Dashboard

Navigate to **http://localhost:5000**

The garage auto-seeds with 12 demo vehicles on first load.

---

## CLI Reference

The C executable works standalone — no server needed:

```bash
cd backend

./parking status                 # Garage overview (JSON)
./parking park ABC123 0          # Park regular vehicle
./parking park VIP001 1          # Park VIP vehicle
./parking lookup ABC123          # Find vehicle location
./parking exit ABC123            # Exit with billing
./parking log                    # Recent activity log
./parking vehicles               # List all parked vehicles
./parking exitlane_push XYZ789   # Push into exit lane
./parking exitlane_view          # View exit lane (LIFO)
./parking exitlane_pop           # Pop + process exit
./parking waitlist               # View waitlist
./parking reset                  # Reset to empty state
```

All commands output JSON to stdout.

---

## API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| `GET`  | `/api/status` | Garage overview (floors, stats) |
| `POST` | `/api/park` | Park a vehicle `{plate, is_vip}` |
| `POST` | `/api/exit` | Exit a vehicle `{plate}` |
| `GET`  | `/api/lookup?plate=X` | Find vehicle location |
| `GET`  | `/api/log` | Recent 20 activity entries |
| `GET`  | `/api/vehicles` | List all parked vehicles with VIP status |
| `POST` | `/api/exitlane_push` | Push to exit lane |
| `POST` | `/api/exitlane_pop` | Pop from exit lane + billing |
| `GET`  | `/api/exitlane_view` | View exit lane contents |
| `GET`  | `/api/waitlist` | View waitlist entries |
| `POST` | `/api/reset` | Reset garage to empty |

---

## Billing & Time

| Setting | Value |
|---------|-------|
| **Time scale** | 10 real seconds = 1 garage hour (360×) |
| **Grace period** | First 15 garage-minutes free |
| **Standard rate** | 10 AED/hour (prorated) |
| **VIP discount** | 20% off total |
| **Daily cap** | 200 AED max |

---

## Themes

Switch themes from the header buttons. Choice is saved to `localStorage`.

| Theme | Style |
|-------|-------|
| **Aurora** | Deep navy/indigo, cyan + violet accents |
| **Ember** | Warm dark reds, orange + pink accents |
| **Frost** | Light glass, indigo + sky blue accents |
| **Sorbet** | Warm pastel peach/pink, purple + orange accents |

---

## Configuration

Default: **3 floors × 20 slots = 60 total**

Edit `backend/include/garage.h` to change:

```c
#define NUM_FLOORS      3
#define SLOTS_PER_FLOOR 20
```

Edit `backend/include/billing.h` to change time scale:

```c
#define TIME_SCALE 360  /* 10 real seconds = 1 garage hour */
```

---

## Testing

| Module | Tests | Status |
|--------|-------|--------|
| Circular Queue | 6 | ✅ Pass |
| Priority Queue | 5 | ✅ Pass |
| Stack | 4 | ✅ Pass |
| Linked List | 4 | ✅ Pass |
| Hash Table | 6 | ✅ Pass |
| Billing | 7 | ✅ Pass |
| **Total** | **33** | **✅ All Pass** |

Compiled with `gcc -Wall -Wextra` — **zero warnings**.

---

## License

This project is licensed under the [MIT License](LICENSE).

© 2026 Dhyey Joshi — built as a portfolio and educational project.
