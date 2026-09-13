/**
 * components.js — UI Component Renderers
 *
 * Pure rendering functions that take data and return HTML strings.
 * These don't manage state or handle events — they just produce
 * the visual output. The app.js controller calls them with fresh
 * data whenever the UI needs to update.
 */

const Components = {

    /**
     * renderSlotGrid — Render the 20-slot grid for a single floor.
     *
     * @param {number} floorId - Floor index (0-based)
     * @param {number} occupied - Number of occupied slots
     * @param {Array}  vehicles - Array of {plate, floor, slot, is_vip} from hash table
     * @returns {string} HTML string for the slot grid
     *
     * We build a lookup map from the vehicles array so we can
     * quickly check if a given slot is occupied and by whom.
     */
    renderSlotGrid(floorId, floorData, vehicles) {
        const totalSlots = floorData.total;

        /*
         * Build a map: slot_index -> vehicle_info for this floor.
         * This lets us mark individual slots as occupied/VIP and
         * show the plate number on hover (tooltip).
         */
        const slotMap = {};
        vehicles.forEach(v => {
            if (v.floor === floorId) {
                slotMap[v.slot] = v;
            }
        });

        let html = '';
        for (let i = 0; i < totalSlots; i++) {
            const vehicle = slotMap[i];
            if (vehicle) {
                const isVip = vehicle.is_vip;
                const cls = isVip ? 'slot--vip' : 'slot--occupied';
                const icon = isVip ? '⭐' : '🚗';
                const tooltip = `${vehicle.plate}${isVip ? ' (VIP)' : ''}`;
                html += `<div class="slot ${cls}" data-tooltip="${tooltip}">${icon}</div>`;
            } else {
                html += `<div class="slot slot--available">${i + 1}</div>`;
            }
        }

        return html;
    },

    /**
     * renderLogList — Render the activity log entries.
     *
     * @param {Array} entries - Array of log entries from the API
     * @returns {string} HTML string
     *
     * We show entries in reverse chronological order (newest first)
     * and format timestamps as relative time strings.
     */
    renderLogList(entries) {
        if (!entries || entries.length === 0) {
            return `
                <div class="empty-state">
                    <div class="empty-state__icon">📭</div>
                    <div class="empty-state__text">No activity yet</div>
                </div>`;
        }

        /* Reverse to show newest first */
        const reversed = [...entries].reverse();

        return reversed.map(entry => {
            const isPark = entry.action === 'park';
            const iconClass = isPark ? 'log-item__icon--park' : 'log-item__icon--exit';
            const icon = isPark ? '🅿️' : '🚪';
            const action = isPark ? 'Parked' : 'Exited';
            const location = `Floor ${entry.floor + 1}, Slot ${entry.slot + 1}`;
            const time = Components.formatTime(entry.timestamp);
            const amount = entry.amount > 0
                ? `<span class="log-item__amount">$${entry.amount.toFixed(2)}</span>`
                : '';

            return `
                <div class="log-item">
                    <div class="log-item__icon ${iconClass}">${icon}</div>
                    <div class="log-item__body">
                        <div class="log-item__plate">${entry.plate}</div>
                        <div class="log-item__detail">${action} · ${location} · ${time}</div>
                    </div>
                    ${amount}
                </div>`;
        }).join('');
    },

    /**
     * renderExitLane — Render the exit lane stack visualization.
     *
     * @param {Array} lane - Array of {plate} objects (top of stack first)
     * @returns {string} HTML string
     *
     * The first item is the car nearest the exit (top of stack).
     * We visually indicate it with a green border and "EXIT →" label.
     */
    renderExitLane(lane) {
        if (!lane || lane.length === 0) {
            return `
                <div class="empty-state">
                    <div class="empty-state__icon">🛣️</div>
                    <div class="empty-state__text">Exit lane is clear</div>
                </div>`;
        }

        return lane.map((car, index) => {
            const position = index + 1;
            return `
                <div class="exit-lane-car">
                    <span class="exit-lane-car__pos">#${position}</span>
                    <span class="exit-lane-car__plate">${car.plate}</span>
                </div>`;
        }).join('');
    },

    /**
     * renderWaitlist — Render the priority waitlist.
     *
     * @param {Array} entries - Array of waitlist entries from the API
     * @returns {string} HTML string
     */
    renderWaitlist(entries) {
        if (!entries || entries.length === 0) {
            return `
                <div class="empty-state">
                    <div class="empty-state__icon">✨</div>
                    <div class="empty-state__text">No vehicles waiting</div>
                </div>`;
        }

        return entries.map(entry => {
            let priorityClass, priorityLabel;
            switch (entry.priority) {
                case 0:
                    priorityClass = 'waitlist-item__priority--vip';
                    priorityLabel = '⭐ VIP';
                    break;
                case 1:
                    priorityClass = 'waitlist-item__priority--reserved';
                    priorityLabel = 'Reserved';
                    break;
                default:
                    priorityClass = 'waitlist-item__priority--regular';
                    priorityLabel = 'Regular';
            }

            return `
                <div class="waitlist-item">
                    <span class="waitlist-item__priority ${priorityClass}">${priorityLabel}</span>
                    <span class="waitlist-item__plate">${entry.plate}</span>
                </div>`;
        }).join('');
    },

    /**
     * renderBillingModal — Create a billing summary popup after exit.
     *
     * @param {Object} data - Exit result from the API
     * @returns {string} HTML for the modal overlay
     */
    renderBillingModal(data) {
        const vipBadge = data.is_vip
            ? '<span style="color: var(--gold); font-size: 0.8rem;"> ⭐ VIP (20% off)</span>'
            : '';

        return `
            <div class="modal-overlay" id="billing-modal">
                <div class="modal">
                    <div class="modal__title">🧾 Exit Receipt ${vipBadge}</div>
                    <div class="modal__subtitle">${data.message}</div>
                    <div class="modal__details">
                        <div class="modal__row">
                            <span class="modal__row-label">License Plate</span>
                            <span class="modal__row-value">${data.plate}</span>
                        </div>
                        <div class="modal__row">
                            <span class="modal__row-label">Location</span>
                            <span class="modal__row-value">Floor ${data.floor + 1}, Slot ${data.slot + 1}</span>
                        </div>
                        <div class="modal__row">
                            <span class="modal__row-label">Duration</span>
                            <span class="modal__row-value">${data.duration_str || '0m'}</span>
                        </div>
                        <div class="modal__row" style="padding-top: 8px; border-top: 1px solid var(--glass-border);">
                            <span class="modal__row-label" style="font-size: 1rem;">Amount Due</span>
                            <span class="modal__row-value modal__row-value--amount">$${data.amount.toFixed(2)}</span>
                        </div>
                    </div>
                    <div class="modal__actions">
                        <button class="btn btn--primary" onclick="document.getElementById('billing-modal').remove()">
                            ✅ Done
                        </button>
                    </div>
                </div>
            </div>`;
    },

    /**
     * renderLookupModal — Show vehicle location after lookup.
     */
    renderLookupModal(data) {
        const vipBadge = data.is_vip
            ? '<span style="color: var(--gold);"> ⭐ VIP</span>'
            : '';
        const parkedSince = Components.formatTime(data.entry_time);

        return `
            <div class="modal-overlay" id="lookup-modal">
                <div class="modal">
                    <div class="modal__title">🔍 Vehicle Found ${vipBadge}</div>
                    <div class="modal__subtitle">${data.message}</div>
                    <div class="modal__details">
                        <div class="modal__row">
                            <span class="modal__row-label">License Plate</span>
                            <span class="modal__row-value">${data.plate}</span>
                        </div>
                        <div class="modal__row">
                            <span class="modal__row-label">Location</span>
                            <span class="modal__row-value">Floor ${data.floor + 1}, Slot ${data.slot + 1}</span>
                        </div>
                        <div class="modal__row">
                            <span class="modal__row-label">Parked Since</span>
                            <span class="modal__row-value">${parkedSince}</span>
                        </div>
                    </div>
                    <div class="modal__actions">
                        <button class="btn btn--ghost" onclick="document.getElementById('lookup-modal').remove()">
                            Close
                        </button>
                    </div>
                </div>
            </div>`;
    },

    /**
     * formatTime — Convert a Unix timestamp to a human-readable string.
     * Shows relative time for recent events, absolute for older ones.
     */
    formatTime(timestamp) {
        if (!timestamp) return 'N/A';

        const now = Math.floor(Date.now() / 1000);
        const diff = now - timestamp;

        if (diff < 60) return 'Just now';
        if (diff < 3600) return `${Math.floor(diff / 60)}m ago`;
        if (diff < 86400) return `${Math.floor(diff / 3600)}h ago`;

        const date = new Date(timestamp * 1000);
        return date.toLocaleString('en-US', {
            month: 'short',
            day: 'numeric',
            hour: 'numeric',
            minute: '2-digit'
        });
    }
};
