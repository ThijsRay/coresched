// Copyright 2024 - Thijs Raymakers
// Licensed under the EUPL v1.2

#include <argp.h>
#include <assert.h>
#include <stdlib.h>
#include <linux/prctl.h>
#include <string.h>
#include <stdbool.h>

static char args_doc[] = "get -p [PID]\n"
			 "create -p [PID]\n"
			 "copy -p [PID] -d [PID]";

static char doc[] = "Manage core scheduling cookies for tasks";

static struct argp_option options[7] = {
	{ 0, 0, 0, 0, "Source PID", 1 },
	{ "pid", 'p', "PID", 0, "source pid of the core scheduling cookie", 1 },
	{ "type", 't', "TYPE", 0,
	  "type of the pid. Can be one of the following: pid, tgid or pgid. Defaults to pgid.",
	  1 },
	{ 0, 0, 0, 0, "Destination PID for copying the core scheduling cookie",
	  2 },
	{ "destpid", 'd', "PID", 0, "pid to copy the core scheduling cookie to",
	  2 },
	{ "desttype", 'e', "TYPE", 0,
	  "type of the destpid. Can be one of the following: pid, tgid or pgid. Defaults to pgid.",
	  2 },
	{ 0 }
};

typedef enum {
	SCHED_CORE_SCOPE_PID = PR_SCHED_CORE_SCOPE_THREAD,
	SCHED_CORE_SCOPE_TGID = PR_SCHED_CORE_SCOPE_THREAD_GROUP,
	SCHED_CORE_SCOPE_PGID = PR_SCHED_CORE_SCOPE_PROCESS_GROUP,
} core_sched_type_t;

typedef enum {
	SCHED_CORE_CMD_GET,
	SCHED_CORE_CMD_CREATE,
	SCHED_CORE_CMD_COPY,
} core_sched_cmd_t;

struct args {
	pid_t from_pid;
	pid_t to_pid;
	core_sched_type_t from_type;
	core_sched_type_t to_type;
	core_sched_cmd_t cmd;
};

bool verify_arguments(struct argp_state *state, struct args *args,
		      char **error_msg)
{
	if (args->from_pid != 0) {
		if (args->cmd == SCHED_CORE_CMD_COPY && args->to_pid == 0) {
			char *msg =
				"Copying a core scheduling cookie requires a destination PID\0";
			const size_t msg_len = strlen(msg);
			*error_msg = malloc(msg_len);
			memcpy(*error_msg, msg, msg_len);
			return false;
		}
		return true;
	}
	char *msg =
		"Retrieving a core scheduling cookie requires a source PID\0";
	const size_t msg_len = strlen(msg);
	*error_msg = malloc(msg_len);
	memcpy(*error_msg, msg, msg_len);
	return false;
}

pid_t parse_pid(struct argp_state *state, char *str)
{
	const int base = 10;
	char *tailptr = NULL;

	pid_t pid = strtol(str, &tailptr, base);

	if (*tailptr == '\0' && tailptr != str) {
		return pid;
	} else {
		argp_error(state, "Failed to parse pid %s", str);
		__builtin_unreachable();
	}
}

core_sched_type_t parse_core_sched_type(struct argp_state *state, char *str)
{
	if (!strncmp(str, "pid\0", 4)) {
		return SCHED_CORE_SCOPE_PID;
	} else if (!strncmp(str, "tgid\0", 5)) {
		return SCHED_CORE_SCOPE_TGID;
	} else if (!strncmp(str, "pgid\0", 5)) {
		return SCHED_CORE_SCOPE_PGID;
	}

	argp_error(state,
		   "'%s' is an invalid option. Must be one of pid/tgid/pgid",
		   str);
	__builtin_unreachable();
}

core_sched_cmd_t parse_cmd(struct argp_state *state, char *arg)
{
	if (!strncmp(arg, "get\0", 4)) {
		return SCHED_CORE_CMD_GET;
	} else if (!strncmp(arg, "create\0", 7)) {
		return SCHED_CORE_CMD_CREATE;
	} else if (!strncmp(arg, "copy\0", 5)) {
		return SCHED_CORE_CMD_COPY;
	} else {
		argp_error(state, "Unknown command '%s'", arg);
		__builtin_unreachable();
	}
}

error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct args *arguments = state->input;

	switch (key) {
	case ARGP_KEY_ARG:
		arguments->cmd = parse_cmd(state, arg);
		break;
	case 'p':
		arguments->from_pid = parse_pid(state, arg);
		break;
	case 't':
		arguments->from_type = parse_core_sched_type(state, arg);
		break;
	case 'd':
		arguments->to_pid = parse_pid(state, arg);
		break;
	case 'e':
		arguments->to_type = parse_core_sched_type(state, arg);
		break;
	case ARGP_KEY_SUCCESS:
		if (state->argc <= 1) {
			argp_usage(state);
		}
		char *error_msg = NULL;
		if (!verify_arguments(state, arguments, &error_msg)) {
			assert(error_msg != NULL);
			argp_error(state, "%s", error_msg);
		}
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

int main(int argc, char *argv[argc])
{
	struct args arguments = { 0 };
	arguments.from_type = SCHED_CORE_SCOPE_PGID;
	arguments.to_type = SCHED_CORE_SCOPE_PGID;

	struct argp argp = { options, parse_opt, args_doc, doc, 0, 0, 0 };

	argp_parse(&argp, argc, argv, 0, 0, &arguments);
	exit(0);
}
