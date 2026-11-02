"""
server.py — Flask Bridge between Frontend and C Backend

This is the glue that connects the web UI to the parking garage C executable.
It does three things:
  1. Serves the frontend static files (HTML/CSS/JS) at http://localhost:5000/
  2. Routes /api/* requests to the C executable via subprocess
  3. Returns the C program's JSON stdout as the HTTP response

The C executable manages all data structures and state internally.
Flask just passes arguments and forwards the output.
"""

from flask import Flask, request, send_from_directory
import subprocess
import os
import json

# Flask app — serves frontend from the 'frontend/' directory
app = Flask(__name__, static_folder='frontend')

# Path to the compiled C executable
BACKEND_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'backend')
PARKING_BIN = os.path.join(BACKEND_DIR, 'parking')

# The C program stores its state file (garage.dat) relative to CWD.
# We set the working directory to the project root so the state file
# lives alongside server.py for easy access.
PROJECT_DIR = os.path.dirname(os.path.abspath(__file__))


# ==================== Frontend Routes ====================

@app.route('/')
def index():
    """Serve the main HTML page."""
    return send_from_directory('frontend', 'index.html')


@app.route('/<path:path>')
def static_files(path):
    """Serve any static file from the frontend directory (CSS, JS, images)."""
    return send_from_directory('frontend', path)


# ==================== API Routes ====================

@app.route('/api/<action>', methods=['GET', 'POST'])
def api(action):
    """
    Route API requests to the C executable.
    
    The C program expects: ./parking <action> [arg1] [arg2] ...
    
    For POST requests, arguments come from the JSON body.
    For GET requests, arguments come from query parameters.
    """
    # Build the command: [./parking, action, arg1, arg2, ...]
    args = [PARKING_BIN, action]

    if request.method == 'POST':
        # POST: extract arguments from JSON body
        data = request.get_json(silent=True) or {}
        # Pass each value as a positional argument to the C program
        for key in ['plate', 'is_vip']:
            if key in data:
                args.append(str(data[key]))
    else:
        # GET: extract arguments from query string
        plate = request.args.get('plate')
        if plate:
            args.append(plate)

    try:
        # Call the C executable and capture its JSON output
        result = subprocess.run(
            args,
            capture_output=True,
            text=True,
            timeout=5,
            cwd=PROJECT_DIR,  # State file lives in the project root
            env={**os.environ, 'GARAGE_STATE_DIR': PROJECT_DIR}
        )

        if result.returncode != 0:
            # C program returned an error
            error_msg = result.stderr.strip() or 'Unknown backend error'
            return app.response_class(
                json.dumps({'status': 'error', 'message': error_msg}),
                mimetype='application/json',
                status=500
            )

        # Return the C program's stdout as-is (it's already JSON)
        return app.response_class(
            result.stdout,
            mimetype='application/json'
        )

    except subprocess.TimeoutExpired:
        return app.response_class(
            json.dumps({'status': 'error', 'message': 'Backend timeout'}),
            mimetype='application/json',
            status=504
        )
    except FileNotFoundError:
        return app.response_class(
            json.dumps({
                'status': 'error',
                'message': 'Backend executable not found. Run: cd backend && make'
            }),
            mimetype='application/json',
            status=500
        )


# ==================== Entry Point ====================

if __name__ == '__main__':
    # Check if the C executable exists
    if not os.path.isfile(PARKING_BIN):
        print(f"\n⚠️  Backend executable not found at: {PARKING_BIN}")
        print(f"   Build it first:  cd backend && make\n")

    print(f"\n🅿️  Parking Garage Management System")
    print(f"   Frontend:  http://localhost:5000")
    print(f"   Backend:   {PARKING_BIN}")
    print(f"   State:     {os.path.join(PROJECT_DIR, 'garage.dat')}\n")

    app.run(debug=True, port=5000)
