// Copyright 2024 - Thijs Raymakers
// Licensed under the EUPL v1.2

#include <argp.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <unistd.h>
#include <error.h>

static char args_doc[] = "get -p PID\n"
			 "create -p PID\n"
			 "copy -p PID -d PID [-t PID]\n"
			 "exec [-p PID] -- PROGRAM ARGS...";

static char doc[] = "Manage core scheduling cookies for tasks";

static struct argp_option options[7] = {
	{ "pid", 'p', "PID", 0,
	  "the PID to get or copy the core scheduling cookie from, or the PID to create the cookie for.",
	  0 },
	{ "dest", 'd', "PID", 0,
	  "the PID to copy the core scheduling cookie to", 0 },
	{ "type", 't', "TYPE", 0,
	  "the type of the destination PID, or the type of the PID to create a core scheduling cookie for. Can be one of the following: pid, tgid or pgid. Defaults to pgid.",
	  0 },
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
	SCHED_CORE_CMD_EXEC,
} core_sched_cmd_t;

struct args {
	pid_t from_pid;
	pid_t to_pid;
	core_sched_type_t type;
	core_sched_cmd_t cmd;
	int exec_argv_offset;
};

unsigned long core_sched_get_cookie(struct args *args)
{
	unsigned long cookie = 0;
	int prctl_errno = prctl(PR_SCHED_CORE, PR_SCHED_CORE_GET,
				args->from_pid, SCHED_CORE_SCOPE_PID, &cookie);
	if (prctl_errno) {
		error(prctl_errno, -prctl_errno,
		      "Failed to get cookie from PID %d", args->from_pid);
	}
	return cookie;
}

void core_sched_create_cookie(struct args *args)
{
	int prctl_errno = prctl(PR_SCHED_CORE, PR_SCHED_CORE_CREATE,
				args->from_pid, args->type, 0);
	if (prctl_errno) {
		error(prctl_errno, -prctl_errno,
		      "Failed to create cookie for PID %d", args->from_pid);
	}
}

void core_sched_pull_cookie(pid_t from)
{
	int prctl_errno = prctl(PR_SCHED_CORE, PR_SCHED_CORE_SHARE_FROM, from,
				SCHED_CORE_SCOPE_PID, 0);
	if (prctl_errno) {
		error(prctl_errno, -prctl_errno,
		      "Failed to pull cookie from PID %d", from);
	}
}

void core_sched_push_cookie(pid_t to, core_sched_type_t type)
{
	int prctl_errno =
		prctl(PR_SCHED_CORE, PR_SCHED_CORE_SHARE_TO, to, type, 0);
	if (prctl_errno) {
		error(prctl_errno, -prctl_errno,
		      "Failed to push cookie to PID %d", to);
	}
}

void core_sched_copy_cookie(struct args *args)
{
	pid_t pid = fork();
	if (pid == -1) {
		error(pid, errno, "Failed to spawn cookie eating child");
	}

	// The child pulls the cookie from the source, and then pushes the
	// cookie to the destination.
	if (!pid) {
		core_sched_pull_cookie(args->from_pid);
		core_sched_push_cookie(args->to_pid, args->type);
	} else {
		int status = 0;
		waitpid(pid, &status, 0);
		if (status) {
			error(status, status,
			      "Failed to copy cookie from %d to %d",
			      args->from_pid, args->to_pid);
		}
	}
}

void core_sched_exec_with_cookie(struct args *args, char **argv)
{
	if (!args->exec_argv_offset) {
		fprintf(stderr,
			"exec has to be followed by a program name to be executed. See '--help' for more info.\n");
		exit(1);
	}

	// Move the argument list to the first argument of the program
	argv = &argv[args->exec_argv_offset];

	pid_t pid = fork();
	if (pid == -1) {
		error(pid, errno, "Failed to spawn cookie eating child");
	}

	if (!pid) {
		// If a source PID is provided, try to copy the cookie from that PID.
		// Otherwise, create a brand new cookie with the provided type.
		if (args->from_pid) {
			core_sched_pull_cookie(args->from_pid);
		} else {
			args->from_pid = getpid();
			core_sched_create_cookie(args);
		}
		unsigned long cookie = core_sched_get_cookie(args);
		fprintf(stderr,
			"spawned pid %d with core scheduling cookie 0x%lx\n",
			getpid(), cookie);
		if (execvp(argv[0], argv)) {
			error(-1, errno, "Failed to spawn process");
		}
	} else {
		exit(0);
	}
}

static const char *copying_requires_dest_msg =
	"Copying a core scheduling cookie requires a destination PID\0";
static const char *retrieve_requires_source_msg =
	"Retrieving a core scheduling cookie requires a source PID\0";
bool verify_arguments(struct args *args, const char **error_msg)
{
	if (args->from_pid != 0 || args->cmd == SCHED_CORE_CMD_EXEC) {
		if (args->cmd == SCHED_CORE_CMD_COPY && args->to_pid == 0) {
			*error_msg = copying_requires_dest_msg;
			return false;
		}
		return true;
	}
	*error_msg = retrieve_requires_source_msg;
	return false;
}

pid_t parse_pid(struct argp_state *state, char *str)
{
	const int base = 10;
	char *tailptr = NULL;

	pid_t pid = strtol(str, &tailptr, base);

	if (*tailptr == '\0' && tailptr != str) {
		if (pid < 0) {
			argp_error(state, "PID %d cannot be negative", pid);
		}
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
	} else if (!strncmp(arg, "exec\0", 5)) {
		return SCHED_CORE_CMD_EXEC;
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
		if (!state->quoted) {
			arguments->cmd = parse_cmd(state, arg);
		} else {
			assert(state->quoted < state->argc);
			arguments->exec_argv_offset = state->quoted;
		}
		break;
	case 'p':
		arguments->from_pid = parse_pid(state, arg);
		break;
	case 't':
		arguments->type = parse_core_sched_type(state, arg);
		break;
	case 'd':
		arguments->to_pid = parse_pid(state, arg);
		break;
	case ARGP_KEY_SUCCESS:
		if (state->argc <= 1) {
			argp_usage(state);
		}
		const char *error_msg = NULL;
		if (!verify_arguments(arguments, &error_msg)) {
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
	arguments.type = SCHED_CORE_SCOPE_PGID;

	struct argp argp = { options, parse_opt, args_doc, doc, 0, 0, 0 };

	argp_parse(&argp, argc, argv, ARGP_IN_ORDER, 0, &arguments);

	unsigned long cookie = 0;
	switch (arguments.cmd) {
	case SCHED_CORE_CMD_GET:
		cookie = core_sched_get_cookie(&arguments);
		if (cookie) {
			printf("core scheduling cookie of pid %d is 0x%lx\n",
			       arguments.from_pid, cookie);
		} else {
			printf("pid %d doesn't have a core scheduling cookie\n",
			       arguments.from_pid);
			exit(1);
		}
		break;
	case SCHED_CORE_CMD_CREATE:
		core_sched_create_cookie(&arguments);
		break;
	case SCHED_CORE_CMD_COPY:
		core_sched_copy_cookie(&arguments);
		break;
	case SCHED_CORE_CMD_EXEC:
		core_sched_exec_with_cookie(&arguments, argv);
		break;
	default:
		exit(1);
	}
}
