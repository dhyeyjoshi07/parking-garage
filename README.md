make# 🅿️ Multi-Level Parking Garage Management System

A full-stack parking garage simulator with a **C backend** (all data structures hand-rolled, no STL) and a **web frontend** (glassmorphism-styled HTML/CSS/JS), connected via a **Python Flask bridge**.

---

## Architecture

```
┌─────────────────────┐     HTTP/JSON      ┌──────────────────┐     subprocess    ┌────────────────────┐
│                     │  ──────────────►   │                  │  ─────────────►  │                    │
│   Browser (UI)      │                    │  Flask Server    │                  │  C Executable      │
│   HTML/CSS/JS       │  ◄──────────────   │  (server.py)     │  ◄─────────────  │  (./parking)       │
│                     │     JSON            │  localhost:5000   │     JSON stdout  │  All data structs  │
└─────────────────────┘                    └──────────────────┘                  └────────┬───────────┘
                                                                                         │
                                                                                    Read/Write
                                                                                         │
                                                                                  ┌──────▼──────┐
                                                                                  │ garage.dat  │
                                                                                  │ (state file)│
                                                                                  └─────────────┘
```

**Why this architecture?**
- The C code stays pure — zero networking, zero HTTP parsing, just data structures and algorithms.
- Flask (~30 lines) acts as a thin bridge: receives HTTP requests, calls the C executable via `subprocess`, and forwards the JSON output.
- State persists in a binary file (`garage.dat`) so each subprocess call is stateless but the garage remembers everything.

---

## Data Structures Used

| Structure | File | Purpose |
|-----------|------|---------|
| **Circular Queue** | `circular_queue.c/h` | Tracks available slot indices per floor. Dequeue to park, enqueue to free. Wraps around for efficient array reuse. |
| **Priority Queue** (Min-Heap) | `priority_queue.c/h` | VIP/reserved waitlist. When the garage is full, vehicles queue by priority (VIP > Reserved > Regular), with FIFO tie-breaking. |
| **Stack** (Linked List) | `stack.c/h` | Exit-lane simulation. Models a single-lane exit where only the last car in can leave first (LIFO). |
| **Linked List** | `linked_list.c/h` | Chronological activity log. Every park/exit event is appended at the tail for O(1) insertion. |
| **Hash Table** (DJB2 + Chaining) | `hash_table.c/h` | O(1) license plate → parking location lookup. Uses DJB2 hash with separate chaining for collisions. |

---

## File Structure

```
parking-garage/
├── backend/
│   ├── include/          # Header files for all modules
│   │   ├── circular_queue.h
│   │   ├── priority_queue.h
│   │   ├── stack.h
│   │   ├── linked_list.h
│   │   ├── hash_table.h
│   │   ├── garage.h      # Core orchestration layer
│   │   ├── billing.h
│   │   └── json_helpers.h
│   ├── src/              # Implementation files
│   │   ├── circular_queue.c
│   │   ├── priority_queue.c
│   │   ├── stack.c
│   │   ├── linked_list.c
│   │   ├── hash_table.c
│   │   ├── garage.c      # Ties all structures together
│   │   ├── billing.c     # Tiered rate calculator
│   │   ├── json_helpers.c
│   │   └── main.c        # CLI entry point
│   ├── tests/
│   │   └── test_all.c    # Unit tests for all structures
│   └── Makefile
├── frontend/             # Web UI (glassmorphism design)
│   ├── index.html
│   ├── css/style.css
│   └── js/
│       ├── app.js
│       ├── api.js
│       └── components.js
├── server.py             # Flask bridge (~30 lines)
├── requirements.txt
├── README.md
└── .gitignore
```

---

## Build & Run Instructions

### Prerequisites
- **GCC** (or any C compiler): `gcc --version`
- **Python 3**: `python3 --version`
- **pip**: `pip3 --version`

### Step 1: Build the C Backend
```bash
cd backend
make          # Compiles to ./parking
```

### Step 2: Run Unit Tests
```bash
cd backend
make test     # Builds and runs test_all
```

### Step 3: Install Python Dependencies
```bash
pip3 install -r requirements.txt
```

### Step 4: Start the Server
```bash
python3 server.py
```

### Step 5: Open the UI
Navigate to **http://localhost:5000** in your browser.

---

## CLI Reference

The C executable can also be used directly from the command line:

```bash
cd backend

# Get garage status
./parking status

# Park a vehicle (0 = regular, 1 = VIP)
./parking park ABC123 0
./parking park VIP001 1

# Look up a vehicle
./parking lookup ABC123

# Exit a vehicle (shows billing)
./parking exit ABC123

# Activity log
./parking log

# Exit lane operations
./parking exitlane_push XYZ789
./parking exitlane_view
./parking exitlane_pop

# View waitlist
./parking waitlist

# Reset to empty
./parking reset
```

All commands output JSON to stdout.

---

## Billing Rates

| Duration | Rate |
|----------|------|
| First 30 minutes | Free |
| 30 min – 2 hours | $3.00/hour |
| 2 – 6 hours | $5.00/hour |
| 6+ hours | $8.00/hour |
| VIP discount | 20% off |
| Daily max cap | $40.00 |

---

## Garage Configuration

Default: **3 floors × 20 slots = 60 total slots**

To change, edit `garage.h`:
```c
#define NUM_FLOORS      3
#define SLOTS_PER_FLOOR 20
```

---

## License

MIT — built as a portfolio/educational project.
