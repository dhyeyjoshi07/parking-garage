/**
 * app.js — Main Application Controller
 *
 * This is the orchestration layer for the frontend. It:
 *   1. Initializes the app on page load
 *   2. Wires up all button click and keyboard events
 *   3. Fetches data from the API and passes it to Components for rendering
 *   4. Manages the active floor tab state
 *   5. Handles toast notifications for user feedback
 *   6. Auto-refreshes every 10 seconds for real-time feel
 */

const App = {
    /* Current state */
    activeFloor: 0,          /* Which floor tab is selected (0-indexed)     */
    statusData: null,        /* Last fetched status response                */
    vehicleList: [],         /* Flat list of all parked vehicles for grid   */
    refreshInterval: null,   /* Auto-refresh timer handle                   */

    /**
     * init — Bootstrap the application on page load.
     */
    async init() {
        /* Wire up event listeners */
        this.bindEvents();

        /* Fetch initial data */
        await this.refreshAll();

        /* Start auto-refresh every 10 seconds */
        this.refreshInterval = setInterval(() => this.refreshAll(), 10000);

        console.log('🅿️ Parking Garage UI initialized');
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
     */
    async handlePark() {
        const plateInput = document.getElementById('park-plate');
        const vipCheckbox = document.getElementById('park-vip');
        const plate = plateInput.value.trim().toUpperCase();

        if (!plate) {
            this.showToast('Please enter a license plate', 'error');
            plateInput.focus();
            return;
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
     */
    async handleExit() {
        const plateInput = document.getElementById('exit-plate');
        const plate = plateInput.value.trim().toUpperCase();

        if (!plate) {
            this.showToast('Please enter a license plate', 'error');
            plateInput.focus();
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
        const plate = plateInput.value.trim().toUpperCase();

        if (!plate) {
            this.showToast('Please enter a license plate', 'error');
            plateInput.focus();
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
     */
    async handleLanePush() {
        const plateInput = document.getElementById('lane-plate');
        const plate = plateInput.value.trim().toUpperCase();

        if (!plate) {
            this.showToast('Please enter a license plate', 'error');
            plateInput.focus();
            return;
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
     * We fetch status, log, exit lane, and waitlist in parallel (Promise.all)
     * since they're independent queries. This minimizes total response time.
     */
    async refreshAll() {
        try {
            const [status, log, lane, waitlist] = await Promise.all([
                API.getStatus(),
                API.getLog(),
                API.getExitLane(),
                API.getWaitlist()
            ]);

            if (status.status === 'ok') {
                this.statusData = status;
                this.updateStats(status);
                this.buildVehicleList(status);
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
        document.getElementById('stat-revenue').textContent = `$${data.total_revenue.toFixed(2)}`;

        const occupancy = data.total_slots > 0
            ? Math.round((data.total_occupied / data.total_slots) * 100)
            : 0;
        document.getElementById('stat-occupancy').textContent = `${occupancy}% occupancy`;
    },

    /**
     * buildVehicleList — Extract all parked vehicles from the status data.
     *
     * The status API doesn't include per-vehicle details, so we use the
     * activity log to reconstruct who is parked where. We track park/exit
     * events to determine current occupancy.
     *
     * NOTE: For a more robust solution, we'd add a dedicated /api/vehicles
     * endpoint. For now, the log-based approach works for the demo.
     */
    buildVehicleList(statusData) {
        /*
         * We don't have a direct vehicle list from the status endpoint,
         * so we'll fetch individual floor/slot data from the log.
         * The vehicleList is populated via the log entries — we track
         * park events that haven't been followed by exit events.
         */
        this.vehicleList = [];

        /* If we have log data cached, rebuild from it */
        if (this._logEntries) {
            const parked = {};
            this._logEntries.forEach(entry => {
                if (entry.action === 'park') {
                    parked[entry.plate] = {
                        plate: entry.plate,
                        floor: entry.floor,
                        slot: entry.slot,
                        is_vip: 0 /* We'll need to detect VIP differently */
                    };
                } else if (entry.action === 'exit') {
                    delete parked[entry.plate];
                }
            });
            this.vehicleList = Object.values(parked);
        }
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
        this._logEntries = data.entries || [];
        const listEl = document.getElementById('log-list');
        listEl.innerHTML = Components.renderLogList(data.entries);

        const countEl = document.getElementById('log-count');
        countEl.textContent = `${data.total || 0} events`;

        /* Also rebuild the vehicle list from log data */
        if (this.statusData) {
            this.buildVehicleList(this.statusData);
            this.updateFloorGrid();
        }
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
     * Toasts auto-dismiss after 3.5 seconds with a fade-out animation.
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
            toast.classList.add('toast--exit');
            setTimeout(() => toast.remove(), 300);
        }, 3500);
    }
};

/* ==================== Bootstrap ==================== */
document.addEventListener('DOMContentLoaded', () => App.init());
