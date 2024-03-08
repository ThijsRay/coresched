// Copyright 2024 - Thijs Raymakers
// Licensed under the EUPL v1.2

#include <argp.h>
#include <stdlib.h>
#include <linux/prctl.h>

static char doc[] = "Manage core scheduling cookies for tasks";

static struct argp_option options[] = {
	{ 0, 0, 0, 0, "Other", -1 },
	{ 0, 0, 0, 0, "Getting core scheduling cookies", 0 },
	{ "get", 'g', "PID", 0, "get the core scheduling cookie of pid", 0 },
	{ "gettype", 0, "TYPE", 0,
	  "type of the pid to get the cookie from. Can be one of the following: pid, tgid or pgid. Defaults to pgid.",
	  0 },
	{ 0, 0, 0, 0, "Creating new core scheduling cookies", 1 },
	{ "new", 'n', "PID", 0, "create a new unique cookie for pid", 1 },
	{ "newtype", 0, "TYPE", 0,
	  "type of the pid for the new cookie. Can be one of the following: pid, tgid or pgid. Defaults to pgid.",
	  1 },
	{ 0, 0, 0, 0, "Copying core scheduling cookies", 2 },
	{ "from", 'f', "PID", 0, "pid to pull the core schedling cookie from",
	  2 },
	{ "fromtype", 0, "TYPE", 0,
	  "type of the 'from' pid. Can be one of the following: pid, tgid or pgid. Defaults to pgid.",
	  2 },
	{ "to", 't', "PID", 0, "pid to push the core schedling cookie to", 2 },
	{ "totype", 0, "TYPE", 0,
	  "type of the 'to' pid. Can be one of the following: pid, tgid or pgid. Defaults to pgid.",
	  2 }
};

typedef enum {
	SCHED_CORE_SCOPE_PID = PR_SCHED_CORE_SCOPE_THREAD,
	SCHED_CORE_SCOPE_TGID = PR_SCHED_CORE_SCOPE_THREAD_GROUP,
	SCHED_CORE_SCOPE_PGID = PR_SCHED_CORE_SCOPE_PROCESS_GROUP,
} core_sched_type_t;

struct args {
	pid_t get_pid;
	pid_t new_pid;
	pid_t from_pid;
	pid_t to_pid;
	core_sched_type_t get_type;
	core_sched_type_t new_type;
	core_sched_type_t from_type;
	core_sched_type_t to_type;
};

error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	switch (key) {
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

int main(int argc, char *argv[argc])
{
	struct args arguments = { 0 };
	struct argp argp = { options, parse_opt, 0, doc, 0, 0, 0 };

	argp_parse(&argp, argc, argv, 0, 0, 0);
	exit(0);
}
