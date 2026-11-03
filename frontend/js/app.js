/**
 * app.js — Main Application Controller
 *
 * This is the orchestration layer for the frontend. It:
 *   1. Runs a terminal boot sequence on first load
 *   2. Seeds the garage with demo vehicles if empty
 *   3. Manages the Aurora glassmorphism theme
 *   4. Generates random plates when inputs are left blank
 *   5. Fetches data from the API and passes it to Components for rendering
 *   6. Manages the active floor tab state
 *   7. Handles toast notifications for user feedback
 *   8. Auto-refreshes every 10 seconds for real-time feel
 */

const App = {
    /* Current state */
    activeFloor: 0,          /* Which floor tab is selected (0-indexed)     */
    statusData: null,        /* Last fetched status response                */
    vehicleList: [],         /* Flat list of all parked vehicles for grid   */
    refreshInterval: null,   /* Auto-refresh timer handle                   */
    garageStartTime: null,   /* Real timestamp when the garage "opened"     */
    clockInterval: null,     /* Garage clock ticker handle                  */

    /* ==================== Random Plate Generator ==================== */

    /**
     * randomPlate — Generate a realistic random license plate.
     *
     * Formats (randomly chosen):
     *   ABC1234   (US-style)
     *   AB12CDE   (EU-style)
     *   XY 4567   (compact)
     *
     * @returns {string} A random plate like "KX4821" or "MN72JTB"
     */
    randomPlate() {
        const letters = 'ABCDEFGHJKLMNPRSTUVWXYZ';  /* No I, O, Q to avoid confusion */
        const digits  = '0123456789';
        const pick = (set, n) => Array.from({ length: n }, () => set[Math.floor(Math.random() * set.length)]).join('');

        const formats = [
            () => pick(letters, 2) + pick(digits, 4),           /* KX4821  */
            () => pick(letters, 3) + pick(digits, 4),           /* ABC1234 */
            () => pick(letters, 2) + pick(digits, 2) + pick(letters, 3),  /* MN72JTB */
        ];

        return formats[Math.floor(Math.random() * formats.length)]();
    },

    /* ==================== Seed Data ==================== */

    /**
     * Seed vehicles — parked on startup if the garage is empty.
     * Mix of regular and VIP cars spread across all 3 floors.
     */
    seedVehicles: [
        { plate: 'VIP001',  vip: true  },
        { plate: 'VIP002',  vip: true  },
        { plate: 'KX4821',  vip: false },
        { plate: 'MN72JTB', vip: false },
        { plate: 'AB1234',  vip: false },
        { plate: 'VIP003',  vip: true  },
        { plate: 'CD5678',  vip: false },
        { plate: 'EF9012',  vip: false },
        { plate: 'GH3456',  vip: false },
        { plate: 'JK7890',  vip: false },
        { plate: 'VIP004',  vip: true  },
        { plate: 'LM2345',  vip: false },
    ],

    /**
     * seedGarage — If the garage is empty, park the seed vehicles.
     * Called once after init. Runs sequentially to avoid race conditions.
     */
    async seedGarage() {
        const status = await API.getStatus();
        if (status.status !== 'ok' || status.total_occupied > 0) {
            return; /* Already has data, skip seeding */
        }

        console.info('🌱 Seeding garage with demo vehicles...');

        for (const car of this.seedVehicles) {
            await API.park(car.plate, car.vip);
        }

        /* Also push a couple into the exit lane for demo */
        await API.exitLanePush('EF9012');
        await API.exitLanePush('GH3456');

        console.info('🌱 Seed complete — 12 vehicles parked, 2 in exit lane');
    },

    /* ==================== Boot Sequence ==================== */

    /**
     * Boot lines displayed one by one in the overlay terminal.
     * Each line is typed out with a small delay for a "system init" effect.
     */
    bootLines: [
        { text: '> parking-garage v3.0.0', cls: 'boot-accent' },
        { text: '  initializing backend process...', cls: 'boot-muted' },
        { text: '  loading hash table (101 buckets)    ✓', cls: '' },
        { text: '  loading circular queue (3 floors)   ✓', cls: '' },
        { text: '  loading exit lane stack             ✓', cls: '' },
        { text: '  loading min-heap waitlist           ✓', cls: '' },
        { text: '  loading billing ledger              ✓', cls: '' },
        { text: '  connecting Flask bridge             ✓', cls: 'boot-accent' },
        { text: '  all subsystems online', cls: 'boot-accent' },
        { text: '', cls: '' },
        { text: '> ready — 60 slots across 3 floors', cls: 'boot-accent' }
    ],

    /**
     * runBootSequence — Animate the terminal boot overlay.
     * Returns a Promise that resolves when boot is complete.
     */
    runBootSequence() {
        return new Promise((resolve) => {
            const overlay = document.getElementById('boot-overlay');
            const container = document.getElementById('boot-lines');

            if (!overlay || !container) {
                resolve();
                return;
            }

            /* Check if user prefers reduced motion */
            if (window.matchMedia('(prefers-reduced-motion: reduce)').matches) {
                overlay.classList.add('off');
                setTimeout(() => { overlay.remove(); resolve(); }, 100);
                return;
            }

            let lineIndex = 0;
            const delay = 120; /* ms between each line */

            const showNextLine = () => {
                if (lineIndex >= this.bootLines.length) {
                    /* All lines shown — dismiss after a short pause */
                    setTimeout(() => {
                        overlay.classList.add('off');
                        setTimeout(() => { overlay.remove(); resolve(); }, 800);
                    }, 600);
                    return;
                }

                const lineData = this.bootLines[lineIndex];
                const el = document.createElement('div');
                el.className = `boot-line visible ${lineData.cls}`;
                el.textContent = lineData.text;
                container.appendChild(el);
                lineIndex++;

                setTimeout(showNextLine, delay);
            };

            /* Start the sequence after a tiny initial delay */
            setTimeout(showNextLine, 300);
        });
    },

    /* ==================== Progress Bar ==================== */

    /**
     * updateProgressBar — Update the thin progress indicator at the top.
     * Based on garage occupancy percentage.
     */
    updateProgressBar() {
        const bar = document.getElementById('progress-bar');
        if (!bar || !this.statusData) return;

        const pct = this.statusData.total_slots > 0
            ? (this.statusData.total_occupied / this.statusData.total_slots) * 100
            : 0;
        bar.style.width = `${pct}%`;
    },

    /* ==================== Garage Clock ==================== */

    /**
     * startGarageClock — Tick the garage time display every second.
     *
     * 10 real seconds = 1 garage hour (TIME_SCALE = 360).
     * The clock shows how much "garage time" has elapsed since page load.
     */
    startGarageClock() {
        const clockEl = document.getElementById('garage-clock');
        if (!clockEl) return;

        const tick = () => {
            const realElapsed = (Date.now() / 1000) - this.garageStartTime;
            const garageSeconds = Math.floor(realElapsed * 360);
            const hours = Math.floor(garageSeconds / 3600);
            const minutes = Math.floor((garageSeconds % 3600) / 60);
            clockEl.textContent = `${hours}h ${minutes}m`;
        };

        tick(); /* Initial update */
        this.clockInterval = setInterval(tick, 1000);
    },

    /* ==================== Initialization ==================== */

    /**
     * init — Bootstrap the application on page load.
     */
    async init() {
        /* Run boot sequence */
        await this.runBootSequence();

        /* Wire up event listeners */
        this.bindEvents();

        /* Seed the garage with demo data if empty */
        await this.seedGarage();

        /* Fetch initial data */
        await this.refreshAll();

        /* Start auto-refresh every 10 seconds */
        this.refreshInterval = setInterval(() => this.refreshAll(), 10000);

        /* Start the garage clock (ticks every second) */
        this.garageStartTime = Date.now() / 1000;
        this.startGarageClock();

        console.info('🅿️ Parking Garage UI initialized');
    },

    /* ==================== Event Binding ==================== */

    bindEvents() {
        /* Park vehicle */
        document.getElementById('btn-park').addEventListener('click', () => this.handlePark());
        document.getElementById('park-plate').addEventListener('keydown', (e) => {
            if (e.key === 'Enter') this.handlePark();
        });

        /* Exit vehicle */
        document.getElementById('btn-exit').addEventListener('click', () => this.handleExit());
        document.getElementById('exit-plate').addEventListener('keydown', (e) => {
            if (e.key === 'Enter') this.handleExit();
        });

        /* Lookup vehicle */
        document.getElementById('btn-lookup').addEventListener('click', () => this.handleLookup());
        document.getElementById('lookup-plate').addEventListener('keydown', (e) => {
            if (e.key === 'Enter') this.handleLookup();
        });

        /* Exit lane */
        document.getElementById('btn-lane-push').addEventListener('click', () => this.handleLanePush());
        document.getElementById('btn-lane-pop').addEventListener('click', () => this.handleLanePop());
        document.getElementById('lane-plate').addEventListener('keydown', (e) => {
            if (e.key === 'Enter') this.handleLanePush();
        });

        /* Header actions */
        document.getElementById('btn-refresh').addEventListener('click', () => {
            this.refreshAll();
            this.showToast('Data refreshed', 'info');
        });
        document.getElementById('btn-reset').addEventListener('click', () => this.handleReset());

        /* Floor tabs — event delegation on the tab container */
        document.getElementById('floor-tabs').addEventListener('click', (e) => {
            const tab = e.target.closest('.floor-tab');
            if (tab) {
                this.activeFloor = parseInt(tab.dataset.floor);
                this.updateFloorTabs();
                this.updateFloorGrid();
            }
        });
    },

    /* ==================== Action Handlers ==================== */

    /**
     * handlePark — Read the form, call the API, update the UI.
     * If the plate field is empty, generate a random plate automatically.
     */
    async handlePark() {
        const plateInput = document.getElementById('park-plate');
        const vipCheckbox = document.getElementById('park-vip');
        let plate = plateInput.value.trim().toUpperCase();

        /* Auto-generate a random plate if left blank */
        if (!plate) {
            plate = this.randomPlate();
            plateInput.value = plate;
        }

        const result = await API.park(plate, vipCheckbox.checked);

        if (result.status === 'ok') {
            this.showToast(`${plate} parked — Floor ${result.floor + 1}, Slot ${result.slot + 1}`, 'success');
            plateInput.value = '';
            vipCheckbox.checked = false;
        } else if (result.status === 'waitlisted') {
            this.showToast(`${plate} added to waitlist (position ${result.waitlist_pos})`, 'info');
            plateInput.value = '';
            vipCheckbox.checked = false;
        } else {
            this.showToast(result.message || 'Park failed', 'error');
        }

        await this.refreshAll();
    },

    /**
     * handleExit — Exit a vehicle and show the billing modal.
     * If the plate field is empty, exit the most recently parked vehicle.
     */
    async handleExit() {
        const plateInput = document.getElementById('exit-plate');
        let plate = plateInput.value.trim().toUpperCase();

        /* If blank, try to use the first vehicle from vehicleList */
        if (!plate && this.vehicleList.length > 0) {
            plate = this.vehicleList[0].plate;
            plateInput.value = plate;
        }

        if (!plate) {
            this.showToast('No vehicles to exit', 'error');
            return;
        }

        const result = await API.exit(plate);

        if (result.status === 'ok') {
            /* Show billing modal */
            document.getElementById('modal-container').innerHTML =
                Components.renderBillingModal(result);
            plateInput.value = '';
        } else {
            this.showToast(result.message || 'Exit failed', 'error');
        }

        await this.refreshAll();
    },

    /**
     * handleLookup — Find a vehicle and show its location.
     */
    async handleLookup() {
        const plateInput = document.getElementById('lookup-plate');
        let plate = plateInput.value.trim().toUpperCase();

        /* If blank, pick a random parked vehicle to look up */
        if (!plate && this.vehicleList.length > 0) {
            const random = this.vehicleList[Math.floor(Math.random() * this.vehicleList.length)];
            plate = random.plate;
            plateInput.value = plate;
        }

        if (!plate) {
            this.showToast('No vehicles to look up', 'error');
            return;
        }

        const result = await API.lookup(plate);

        if (result.status === 'ok') {
            /* Show lookup modal */
            document.getElementById('modal-container').innerHTML =
                Components.renderLookupModal(result);

            /* Also switch to the floor where the vehicle is */
            this.activeFloor = result.floor;
            this.updateFloorTabs();
            this.updateFloorGrid();
            plateInput.value = '';
        } else {
            this.showToast(result.message || 'Vehicle not found', 'error');
        }
    },

    /**
     * handleLanePush — Push a vehicle into the exit lane.
     * If the plate field is empty, generate a random plate.
     */
    async handleLanePush() {
        const plateInput = document.getElementById('lane-plate');
        let plate = plateInput.value.trim().toUpperCase();

        /* Auto-generate if blank — pick a random parked vehicle */
        if (!plate && this.vehicleList.length > 0) {
            const random = this.vehicleList[Math.floor(Math.random() * this.vehicleList.length)];
            plate = random.plate;
            plateInput.value = plate;
        } else if (!plate) {
            plate = this.randomPlate();
            plateInput.value = plate;
        }

        const result = await API.exitLanePush(plate);

        if (result.status === 'ok') {
            this.showToast(`${plate} pushed to exit lane`, 'success');
            plateInput.value = '';
        } else {
            this.showToast(result.message || 'Push failed', 'error');
        }

        await this.refreshAll();
    },

    /**
     * handleLanePop — Pop the top vehicle and process exit + billing.
     * No plate needed — always pops from top of stack.
     */
    async handleLanePop() {
        const result = await API.exitLanePop();

        if (result.status === 'ok') {
            if (result.amount !== undefined) {
                /* Show billing modal for the popped vehicle */
                document.getElementById('modal-container').innerHTML =
                    Components.renderBillingModal({
                        ...result,
                        floor: result.floor || 0,
                        slot: result.slot || 0,
                        duration_str: result.duration_str || '0m'
                    });
            }
            this.showToast(`${result.plate} exited from lane`, 'success');
        } else {
            this.showToast(result.message || 'Pop failed', 'error');
        }

        await this.refreshAll();
    },

    /**
     * handleReset — Reset the entire garage after confirmation.
     */
    async handleReset() {
        /* Show a confirmation modal */
        document.getElementById('modal-container').innerHTML = `
            <div class="modal-overlay" id="reset-modal">
                <div class="modal">
                    <div class="modal__title">⚠️ Reset Garage</div>
                    <div class="modal__subtitle">
                        This will remove all parked vehicles, clear the waitlist,
                        exit lane, and activity log. This cannot be undone.
                    </div>
                    <div class="modal__actions">
                        <button class="btn btn--ghost" onclick="document.getElementById('reset-modal').remove()">
                            Cancel
                        </button>
                        <button class="btn btn--coral" id="btn-confirm-reset">
                            🗑️ Reset Everything
                        </button>
                    </div>
                </div>
            </div>`;

        document.getElementById('btn-confirm-reset').addEventListener('click', async () => {
            document.getElementById('reset-modal').remove();
            const result = await API.reset();
            if (result.status === 'ok') {
                this.showToast('Garage reset to empty', 'success');
            } else {
                this.showToast('Reset failed', 'error');
            }
            await this.refreshAll();
        });
    },

    /* ==================== Data Refresh ==================== */

    /**
     * refreshAll — Fetch all data from the backend and update every UI section.
     *
     * We fetch status, vehicles, log, exit lane, and waitlist in parallel
     * (Promise.all) since they're independent queries.
     */
    async refreshAll() {
        try {
            const [status, vehicles, log, lane, waitlist] = await Promise.all([
                API.getStatus(),
                API.getVehicles(),
                API.getLog(),
                API.getExitLane(),
                API.getWaitlist()
            ]);

            if (status.status === 'ok') {
                this.statusData = status;
                this.updateStats(status);
                this.updateProgressBar();
            }

            if (vehicles.status === 'ok') {
                this.vehicleList = vehicles.vehicles || [];
                this.updateFloorGrid();
            }

            if (log.status === 'ok') {
                this.updateLog(log);
            }

            if (lane.status === 'ok') {
                this.updateExitLane(lane);
            }

            if (waitlist.status === 'ok') {
                this.updateWaitlist(waitlist);
            }
        } catch (error) {
            console.error('Refresh failed:', error);
        }
    },

    /* ==================== UI Update Functions ==================== */

    /**
     * updateStats — Update the top stats bar numbers.
     */
    updateStats(data) {
        document.getElementById('stat-total').textContent = data.total_slots;
        document.getElementById('stat-occupied').textContent = data.total_occupied;
        document.getElementById('stat-available').textContent = data.total_available;
        document.getElementById('stat-waitlist').textContent = data.waitlist_size;
        document.getElementById('stat-revenue').textContent = `${data.total_revenue.toFixed(2)} AED`;

        const occupancy = data.total_slots > 0
            ? Math.round((data.total_occupied / data.total_slots) * 100)
            : 0;
        document.getElementById('stat-occupancy').textContent = `${occupancy}% occupancy`;
    },

    /**
     * updateFloorTabs — Highlight the active floor tab.
     */
    updateFloorTabs() {
        const tabs = document.querySelectorAll('.floor-tab');
        tabs.forEach(tab => {
            const floorId = parseInt(tab.dataset.floor);
            if (floorId === this.activeFloor) {
                tab.classList.add('floor-tab--active');
            } else {
                tab.classList.remove('floor-tab--active');
            }
        });
    },

    /**
     * updateFloorGrid — Redraw the slot grid for the active floor.
     */
    updateFloorGrid() {
        if (!this.statusData) return;

        const floorData = this.statusData.floors[this.activeFloor];
        if (!floorData) return;

        /* Update floor info text and progress bar */
        const infoText = document.getElementById('floor-info-text');
        const infoFill = document.getElementById('floor-info-fill');
        infoText.textContent = `${floorData.occupied} / ${floorData.total} occupied`;

        const fillPct = (floorData.occupied / floorData.total) * 100;
        infoFill.style.width = `${fillPct}%`;

        /* Change fill color based on occupancy */
        if (fillPct >= 90) {
            infoFill.style.background = 'linear-gradient(90deg, var(--coral), #ff4444)';
        } else if (fillPct >= 70) {
            infoFill.style.background = 'linear-gradient(90deg, var(--gold), var(--coral))';
        } else {
            infoFill.style.background = 'linear-gradient(90deg, var(--cyan), var(--purple))';
        }

        /* Render the slot grid */
        const gridEl = document.getElementById('slot-grid');
        gridEl.innerHTML = Components.renderSlotGrid(
            this.activeFloor, floorData, this.vehicleList
        );

        /* Update tab badges with per-floor occupied count */
        const tabs = document.querySelectorAll('.floor-tab');
        this.statusData.floors.forEach((floor, idx) => {
            if (tabs[idx]) {
                tabs[idx].textContent = `Floor ${idx + 1} (${floor.occupied}/${floor.total})`;
            }
        });
    },

    /**
     * updateLog — Redraw the activity log.
     */
    updateLog(data) {
        const listEl = document.getElementById('log-list');
        listEl.innerHTML = Components.renderLogList(data.entries);

        const countEl = document.getElementById('log-count');
        countEl.textContent = `${data.total || 0} events`;
    },

    /**
     * updateExitLane — Redraw the exit lane stack.
     */
    updateExitLane(data) {
        const stackEl = document.getElementById('exit-lane-stack');
        stackEl.innerHTML = Components.renderExitLane(data.lane);

        const countEl = document.getElementById('lane-count');
        countEl.textContent = `${data.lane_size || 0} cars`;
    },

    /**
     * updateWaitlist — Redraw the waitlist panel.
     */
    updateWaitlist(data) {
        const listEl = document.getElementById('waitlist-list');
        listEl.innerHTML = Components.renderWaitlist(data.entries);

        const countEl = document.getElementById('waitlist-count');
        countEl.textContent = `${data.waitlist_size || 0} waiting`;
    },

    /* ==================== Toast Notifications ==================== */

    /**
     * showToast — Display a temporary notification.
     *
     * @param {string} message - Text to show
     * @param {string} type    - 'success', 'error', or 'info'
     *
     * Toasts auto-dismiss after 3.5 seconds with a fade-out.
     */
    showToast(message, type = 'info') {
        const container = document.getElementById('toast-container');
        const icons = { success: '✅', error: '❌', info: 'ℹ️' };

        const toast = document.createElement('div');
        toast.className = `toast toast--${type}`;
        toast.innerHTML = `<span>${icons[type] || ''}</span> ${message}`;

        container.appendChild(toast);

        /* Auto-dismiss after 3.5 seconds */
        setTimeout(() => {
            toast.style.opacity = '0';
            toast.style.transform = 'translateX(40px)';
            setTimeout(() => toast.remove(), 300);
        }, 3500);
    }
};

/* ==================== Bootstrap ==================== */
document.addEventListener('DOMContentLoaded', () => App.init());
