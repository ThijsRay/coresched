// Copyright 2024 - Thijs Raymakers
// Licensed under the EUPL v1.2

#include <argp.h>
#include <stdlib.h>
#include <linux/prctl.h>
#include <string.h>

static char doc[] = "Manage core scheduling cookies for tasks";

#define GETTYPE_OPT 1
#define NEWTYPE_OPT 2
#define FROMTYPE_OPT 3
#define TOTYPE_OPT 4

static struct argp_option options[] = {
	{ 0, 0, 0, 0, "Getting core scheduling cookies", 1 },
	{ "get", 'g', "PID", 0, "get the core scheduling cookie of pid", 1 },
	{ "gettype", GETTYPE_OPT, "TYPE", 0,
	  "type of the pid to get the cookie from. Can be one of the following: pid, tgid or pgid. Defaults to pgid.",
	  1 },
	{ 0, 0, 0, 0, "Creating new core scheduling cookies", 2 },
	{ "new", 'n', "PID", 0, "create a new unique cookie for pid", 2 },
	{ "newtype", NEWTYPE_OPT, "TYPE", 0,
	  "type of the pid for the new cookie. Can be one of the following: pid, tgid or pgid. Defaults to pgid.",
	  2 },
	{ 0, 0, 0, 0, "Copying core scheduling cookies", 3 },
	{ "from", 'f', "PID", 0, "pid to pull the core schedling cookie from",
	  3 },
	{ "fromtype", FROMTYPE_OPT, "TYPE", 0,
	  "type of the 'from' pid. Can be one of the following: pid, tgid or pgid. Defaults to pgid.",
	  3 },
	{ "to", 't', "PID", 0, "pid to push the core schedling cookie to", 3 },
	{ "totype", TOTYPE_OPT, "TYPE", 0,
	  "type of the 'to' pid. Can be one of the following: pid, tgid or pgid. Defaults to pgid.",
	  3 },
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

pid_t parse_pid(char *str)
{
	const int base = 10;
	char *tailptr = NULL;

	pid_t pid = strtol(str, &tailptr, base);

	if (*tailptr == '\0' && tailptr != str) {
		return pid;
	} else {
		fprintf(stderr, "Failed to parse pid %s\n", str);
		exit(1);
	}
}

core_sched_type_t parse_core_sched_type(char *str)
{
	if (!strncmp(str, "pid\0", 4)) {
		return SCHED_CORE_SCOPE_PID;
	} else if (!strncmp(str, "tgid\0", 5)) {
		return SCHED_CORE_SCOPE_TGID;
	} else if (!strncmp(str, "pgid\0", 5)) {
		return SCHED_CORE_SCOPE_PGID;
	}

	fprintf(stderr,
		"'%s' is an invalid option. Must be one of pid/tgid/pgid\n",
		str);
	exit(1);
}

error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct args *arguments = state->input;

	switch (key) {
	case 'g':
		arguments->get_pid = parse_pid(arg);
		break;
	case GETTYPE_OPT:
		arguments->get_type = parse_core_sched_type(arg);
		break;
	case 'n':
		arguments->new_pid = parse_pid(arg);
		break;
	case NEWTYPE_OPT:
		arguments->new_type = parse_core_sched_type(arg);
		break;
	case 'f':
		arguments->from_pid = parse_pid(arg);
		break;
	case FROMTYPE_OPT:
		arguments->from_type = parse_core_sched_type(arg);
		break;
	case 't':
		arguments->to_pid = parse_pid(arg);
		break;
	case TOTYPE_OPT:
		arguments->to_type = parse_core_sched_type(arg);
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

int main(int argc, char *argv[argc])
{
	struct args arguments = { 0 };
	struct argp argp = { options, parse_opt, 0, doc, 0, 0, 0 };

	argp_parse(&argp, argc, argv, 0, 0, &arguments);
	exit(0);
}
