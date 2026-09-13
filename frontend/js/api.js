/**
 * api.js — Backend API Client
 *
 * Wraps all fetch calls to the Flask backend (/api/*).
 * Each function returns a Promise that resolves to the parsed JSON response.
 *
 * The Flask server routes these to the C executable and returns its
 * JSON stdout. We just need to call the right endpoint with the right data.
 */

const API = {
    /** Base URL — empty string means same origin (Flask serves both UI and API) */
    BASE: '',

    /**
     * _request — Internal helper for all API calls.
     * Handles both GET and POST, parses JSON, and provides error handling.
     */
    async _request(method, endpoint, data = null) {
        const options = {
            method,
            headers: {}
        };

        if (data && method === 'POST') {
            options.headers['Content-Type'] = 'application/json';
            options.body = JSON.stringify(data);
        }

        try {
            const response = await fetch(`${this.BASE}/api/${endpoint}`, options);
            const json = await response.json();
            return json;
        } catch (error) {
            console.error(`API error [${endpoint}]:`, error);
            return { status: 'error', message: 'Connection to backend failed' };
        }
    },

    /** Get the full garage status (floors, occupancy, stats) */
    getStatus() {
        return this._request('GET', 'status');
    },

    /** Park a vehicle */
    park(plate, isVip) {
        return this._request('POST', 'park', {
            plate: plate.toUpperCase().trim(),
            is_vip: isVip ? 1 : 0
        });
    },

    /** Exit a vehicle (triggers billing) */
    exit(plate) {
        return this._request('POST', 'exit', {
            plate: plate.toUpperCase().trim()
        });
    },

    /** Look up a vehicle by plate */
    lookup(plate) {
        return this._request('GET', `lookup?plate=${encodeURIComponent(plate.toUpperCase().trim())}`);
    },

    /** Get the activity log (recent 20 entries) */
    getLog() {
        return this._request('GET', 'log');
    },

    /** Get exit lane contents */
    getExitLane() {
        return this._request('GET', 'exitlane_view');
    },

    /** Push a vehicle into the exit lane */
    exitLanePush(plate) {
        return this._request('POST', 'exitlane_push', {
            plate: plate.toUpperCase().trim()
        });
    },

    /** Pop the top vehicle from the exit lane (triggers billing) */
    exitLanePop() {
        return this._request('POST', 'exitlane_pop');
    },

    /** Get the waitlist */
    getWaitlist() {
        return this._request('GET', 'waitlist');
    },

    /** Reset the garage to empty */
    reset() {
        return this._request('POST', 'reset');
    }
};
