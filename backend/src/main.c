/*
 * main.c — CLI Entry Point for the Parking Garage Backend
 *
 * PURPOSE:
 *   This program runs as a command-line tool. Flask (server.py) calls it
 *   via subprocess for each API request. The program:
 *     1. Loads garage state from garage.dat (or initializes fresh)
 *     2. Parses the command and arguments
 *     3. Executes the operation
 *     4. Prints JSON to stdout
 *     5. Saves updated state back to garage.dat
 *
 * USAGE:
 *   ./parking <command> [args...]
 *
 *   Commands:
 *     status                         — Get garage overview
 *     park <plate> <is_vip>          — Park a vehicle
 *     exit <plate>                   — Exit a vehicle (with billing)
 *     lookup <plate>                 — Find a vehicle's location
 *     log                            — Get recent activity log
 *     exitlane_push <plate>          — Push vehicle into exit lane
 *     exitlane_pop                   — Pop top vehicle from exit lane
 *     exitlane_view                  — View exit lane contents
 *     waitlist                       — View waitlist contents
 *     reset                          — Reset garage to empty state
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/garage.h"
#include "../include/json_helpers.h"
#include "../include/billing.h"

/*
 * get_state_path — Determine the path to the state file.
 *
 * Uses the GARAGE_STATE_DIR environment variable if set (Flask sets this
 * to the project root), otherwise falls back to the current directory.
 */
static void get_state_path(char *path, int size)
{
    const char *dir = getenv("GARAGE_STATE_DIR");
    if (dir && strlen(dir) > 0) {
        snprintf(path, size, "%s/%s", dir, STATE_FILE);
    } else {
        snprintf(path, size, "%s", STATE_FILE);
    }
}

/* ==================== Command Handlers ==================== */

/*
 * cmd_status — Print the garage overview as JSON.
 *
 * Shows per-floor occupancy, waitlist size, exit lane size, and revenue.
 */
static void cmd_status(Garage *g)
{
    JsonBuilder jb;
    int i;

    json_init(&jb);
    json_object_start(&jb, NULL);

    json_add_string(&jb, "status", "ok");

    /* Per-floor breakdown */
    json_array_start(&jb, "floors");
    for (i = 0; i < NUM_FLOORS; i++) {
        int available = cq_available_count(&g->floors[i]);
        int occupied = SLOTS_PER_FLOOR - available;

        json_object_start(&jb, NULL);
        json_add_int(&jb, "id", i);
        json_add_int(&jb, "total", SLOTS_PER_FLOOR);
        json_add_int(&jb, "occupied", occupied);
        json_add_int(&jb, "available", available);
        json_object_end(&jb);
    }
    json_array_end(&jb);

    /* Summary stats */
    {
        int total_occupied = ht_count(&g->vehicles);
        json_add_int(&jb, "total_slots", TOTAL_SLOTS);
        json_add_int(&jb, "total_occupied", total_occupied);
        json_add_int(&jb, "total_available", TOTAL_SLOTS - total_occupied);
        json_add_int(&jb, "waitlist_size", pq_size(&g->waitlist));
        json_add_int(&jb, "exit_lane_size", stack_size(&g->exit_lane));
        json_add_double(&jb, "total_revenue",
                         (double)g->total_revenue_cents / 100.0);
    }

    json_object_end(&jb);
    printf("%s\n", json_finish(&jb));
}

/*
 * cmd_park — Park a vehicle in the nearest available slot.
 */
static void cmd_park(Garage *g, const char *plate, int is_vip)
{
    JsonBuilder jb;
    ParkResult result;

    garage_park(g, plate, is_vip, &result);

    json_init(&jb);
    json_object_start(&jb, NULL);

    if (result.result_code == GARAGE_OK) {
        json_add_string(&jb, "status", "ok");
    } else if (result.result_code == GARAGE_ERR_WAITLIST) {
        json_add_string(&jb, "status", "waitlisted");
    } else {
        json_add_string(&jb, "status", "error");
    }

    json_add_string(&jb, "plate", plate);
    json_add_int(&jb, "floor", result.floor);
    json_add_int(&jb, "slot", result.slot);
    json_add_int(&jb, "waitlist_pos", result.waitlist_pos);
    json_add_int(&jb, "is_vip", is_vip);
    json_add_string(&jb, "message", result.message);

    json_object_end(&jb);
    printf("%s\n", json_finish(&jb));
}

/*
 * cmd_exit — Exit a vehicle and show billing.
 */
static void cmd_exit(Garage *g, const char *plate)
{
    JsonBuilder jb;
    ExitResult result;

    garage_exit(g, plate, &result);

    json_init(&jb);
    json_object_start(&jb, NULL);

    if (result.result_code == GARAGE_OK) {
        json_add_string(&jb, "status", "ok");
    } else {
        json_add_string(&jb, "status", "error");
    }

    json_add_string(&jb, "plate", result.plate);
    json_add_int(&jb, "floor", result.floor);
    json_add_int(&jb, "slot", result.slot);
    json_add_long(&jb, "entry_time", result.entry_time);
    json_add_long(&jb, "exit_time", result.exit_time);
    json_add_long(&jb, "duration_seconds", result.duration_seconds);
    json_add_string(&jb, "duration_str", result.duration_str);
    json_add_double(&jb, "amount", result.amount);
    json_add_int(&jb, "is_vip", result.is_vip);
    json_add_string(&jb, "message", result.message);

    json_object_end(&jb);
    printf("%s\n", json_finish(&jb));
}

/*
 * cmd_lookup — Find where a vehicle is parked.
 */
static void cmd_lookup(Garage *g, const char *plate)
{
    JsonBuilder jb;
    LookupResult result;

    garage_lookup(g, plate, &result);

    json_init(&jb);
    json_object_start(&jb, NULL);

    if (result.result_code == GARAGE_OK) {
        json_add_string(&jb, "status", "ok");
    } else {
        json_add_string(&jb, "status", "error");
    }

    json_add_string(&jb, "plate", result.plate);
    json_add_int(&jb, "floor", result.floor);
    json_add_int(&jb, "slot", result.slot);
    json_add_long(&jb, "entry_time", result.entry_time);
    json_add_int(&jb, "is_vip", result.is_vip);
    json_add_string(&jb, "message", result.message);

    json_object_end(&jb);
    printf("%s\n", json_finish(&jb));
}

/*
 * cmd_log — Print the 20 most recent activity log entries.
 */
static void cmd_log(Garage *g)
{
    JsonBuilder jb;
    LogEntry *entries[20];
    int count, i;

    count = ll_get_recent(&g->log, entries, 20);

    json_init(&jb);
    json_object_start(&jb, NULL);
    json_add_string(&jb, "status", "ok");

    json_array_start(&jb, "entries");
    for (i = 0; i < count; i++) {
        json_object_start(&jb, NULL);
        json_add_string(&jb, "plate", entries[i]->plate);
        json_add_string(&jb, "action", entries[i]->action);
        json_add_int(&jb, "floor", entries[i]->floor);
        json_add_int(&jb, "slot", entries[i]->slot);
        json_add_long(&jb, "timestamp", entries[i]->timestamp);
        json_add_double(&jb, "amount", entries[i]->amount);
        json_object_end(&jb);
    }
    json_array_end(&jb);

    json_add_int(&jb, "total", ll_count(&g->log));

    json_object_end(&jb);
    printf("%s\n", json_finish(&jb));
}

/*
 * cmd_exitlane_push — Push a vehicle into the exit lane.
 */
static void cmd_exitlane_push(Garage *g, const char *plate)
{
    JsonBuilder jb;

    json_init(&jb);
    json_object_start(&jb, NULL);

    if (stack_push(&g->exit_lane, plate) == 0) {
        json_add_string(&jb, "status", "ok");
        json_add_string(&jb, "message", "Vehicle added to exit lane");
    } else {
        json_add_string(&jb, "status", "error");
        json_add_string(&jb, "message", "Failed to add vehicle to exit lane");
    }

    json_add_int(&jb, "lane_size", stack_size(&g->exit_lane));

    json_object_end(&jb);
    printf("%s\n", json_finish(&jb));
}

/*
 * cmd_exitlane_pop — Pop the top vehicle from the exit lane.
 *
 * After popping, we also trigger garage_exit to process billing
 * and free the slot.
 */
static void cmd_exitlane_pop(Garage *g)
{
    JsonBuilder jb;
    char plate[16];

    json_init(&jb);
    json_object_start(&jb, NULL);

    if (stack_pop(&g->exit_lane, plate) == 0) {
        json_add_string(&jb, "status", "ok");
        json_add_string(&jb, "plate", plate);
        json_add_string(&jb, "message", "Vehicle removed from exit lane");
        json_add_int(&jb, "lane_size", stack_size(&g->exit_lane));

        /*
         * Also process the actual exit (billing, slot release).
         * We embed the exit result in the same response.
         */
        {
            ExitResult exit_result;
            garage_exit(g, plate, &exit_result);
            if (exit_result.result_code == GARAGE_OK) {
                json_add_double(&jb, "amount", exit_result.amount);
                json_add_string(&jb, "duration_str", exit_result.duration_str);
                json_add_long(&jb, "duration_seconds",
                              exit_result.duration_seconds);
            }
        }
    } else {
        json_add_string(&jb, "status", "error");
        json_add_string(&jb, "message", "Exit lane is empty");
        json_add_int(&jb, "lane_size", 0);
    }

    json_object_end(&jb);
    printf("%s\n", json_finish(&jb));
}

/*
 * cmd_exitlane_view — Display all vehicles in the exit lane.
 *
 * We walk the stack's linked list from top to bottom without modifying it.
 * (This accesses the internal structure directly — acceptable since
 * we own all the code.)
 */
static void cmd_exitlane_view(Garage *g)
{
    JsonBuilder jb;
    StackNode *node;

    json_init(&jb);
    json_object_start(&jb, NULL);
    json_add_string(&jb, "status", "ok");

    json_array_start(&jb, "lane");
    node = g->exit_lane.top;
    while (node) {
        json_object_start(&jb, NULL);
        json_add_string(&jb, "plate", node->plate);
        json_object_end(&jb);
        node = node->next;
    }
    json_array_end(&jb);

    json_add_int(&jb, "lane_size", stack_size(&g->exit_lane));

    json_object_end(&jb);
    printf("%s\n", json_finish(&jb));
}

/*
 * cmd_waitlist — Display all vehicles in the waitlist.
 *
 * We read directly from the heap array. Note: the array is in heap
 * order, not sorted order, but for display purposes this is fine.
 * The frontend can sort by priority if needed.
 */
static void cmd_waitlist(Garage *g)
{
    JsonBuilder jb;
    int i;
    const char *priority_names[] = {"VIP", "Reserved", "Regular"};

    json_init(&jb);
    json_object_start(&jb, NULL);
    json_add_string(&jb, "status", "ok");

    json_array_start(&jb, "entries");
    for (i = 0; i < g->waitlist.size; i++) {
        json_object_start(&jb, NULL);
        json_add_string(&jb, "plate", g->waitlist.heap[i].plate);
        json_add_int(&jb, "priority", g->waitlist.heap[i].priority);
        json_add_string(&jb, "priority_name",
                         priority_names[g->waitlist.heap[i].priority]);
        json_add_long(&jb, "request_time",
                       g->waitlist.heap[i].request_time);
        json_add_int(&jb, "is_vip", g->waitlist.heap[i].is_vip);
        json_object_end(&jb);
    }
    json_array_end(&jb);

    json_add_int(&jb, "waitlist_size", pq_size(&g->waitlist));

    json_object_end(&jb);
    printf("%s\n", json_finish(&jb));
}

/*
 * cmd_reset — Delete the state file and reinitialize.
 */
static void cmd_reset(Garage *g, const char *state_path)
{
    JsonBuilder jb;

    /* Destroy current state, reinitialize, and save fresh */
    garage_destroy(g);
    garage_init(g);
    garage_save(g, state_path);

    json_init(&jb);
    json_object_start(&jb, NULL);
    json_add_string(&jb, "status", "ok");
    json_add_string(&jb, "message", "Garage reset to empty state");
    json_object_end(&jb);
    printf("%s\n", json_finish(&jb));
}

/* ==================== Usage ==================== */

static void print_usage(void)
{
    fprintf(stderr, "Usage: parking <command> [args...]\n");
    fprintf(stderr, "Commands:\n");
    fprintf(stderr, "  status                  — Garage overview\n");
    fprintf(stderr, "  park <plate> <is_vip>   — Park a vehicle\n");
    fprintf(stderr, "  exit <plate>            — Exit a vehicle\n");
    fprintf(stderr, "  lookup <plate>          — Find a vehicle\n");
    fprintf(stderr, "  log                     — Activity log\n");
    fprintf(stderr, "  exitlane_push <plate>   — Add to exit lane\n");
    fprintf(stderr, "  exitlane_pop            — Remove from exit lane\n");
    fprintf(stderr, "  exitlane_view           — View exit lane\n");
    fprintf(stderr, "  waitlist                — View waitlist\n");
    fprintf(stderr, "  reset                   — Reset to empty\n");
}

/* ==================== Main ==================== */

int main(int argc, char *argv[])
{
    Garage g;
    char state_path[256];
    int needs_save = 1;  /* Most commands modify state */

    if (argc < 2) {
        print_usage();
        return 1;
    }

    get_state_path(state_path, sizeof(state_path));

    /*
     * Try to load existing state. If the file doesn't exist or is
     * corrupt, initialize a fresh garage.
     */
    if (garage_load(&g, state_path) != 0) {
        if (garage_init(&g) != 0) {
            fprintf(stderr, "Failed to initialize garage\n");
            return 1;
        }
    }

    /* Dispatch to the appropriate command handler */
    if (strcmp(argv[1], "status") == 0) {
        cmd_status(&g);
        needs_save = 0;  /* Read-only operation */

    } else if (strcmp(argv[1], "park") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: parking park <plate> <is_vip>\n");
            garage_destroy(&g);
            return 1;
        }
        cmd_park(&g, argv[2], atoi(argv[3]));

    } else if (strcmp(argv[1], "exit") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: parking exit <plate>\n");
            garage_destroy(&g);
            return 1;
        }
        cmd_exit(&g, argv[2]);

    } else if (strcmp(argv[1], "lookup") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: parking lookup <plate>\n");
            garage_destroy(&g);
            return 1;
        }
        cmd_lookup(&g, argv[2]);
        needs_save = 0;

    } else if (strcmp(argv[1], "log") == 0) {
        cmd_log(&g);
        needs_save = 0;

    } else if (strcmp(argv[1], "exitlane_push") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: parking exitlane_push <plate>\n");
            garage_destroy(&g);
            return 1;
        }
        cmd_exitlane_push(&g, argv[2]);

    } else if (strcmp(argv[1], "exitlane_pop") == 0) {
        cmd_exitlane_pop(&g);

    } else if (strcmp(argv[1], "exitlane_view") == 0) {
        cmd_exitlane_view(&g);
        needs_save = 0;

    } else if (strcmp(argv[1], "waitlist") == 0) {
        cmd_waitlist(&g);
        needs_save = 0;

    } else if (strcmp(argv[1], "reset") == 0) {
        cmd_reset(&g, state_path);
        garage_destroy(&g);
        return 0;  /* Already saved inside cmd_reset */

    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        print_usage();
        garage_destroy(&g);
        return 1;
    }

    /* Save state after any modifying operation */
    if (needs_save) {
        garage_save(&g, state_path);
    }

    garage_destroy(&g);
    return 0;
}
