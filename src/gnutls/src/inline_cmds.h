/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef GNUTLS_SRC_INLINE_CMDS_H
#define GNUTLS_SRC_INLINE_CMDS_H

/* 
 * The inline commands is a facility that can be used optionally
 * when --inline-commands is set during invocation of gnutls-cli
 * to send inline commands at any time while a secure connection
 * between the client and server is active. This is especially 
 * useful when the HTTPS connection is (HTTP) persistent -  
 * inline commands can be issued between HTTP requests, ex: GET.
 * session renegotiation and session resumption can be issued 
 * inline between GET requests.
 *
 * Following inline commands are currently supported:
 * ^resume^      - perform session resumption (similar to option -r)
 * ^renegotiate^ - perform session renegotiation (similar to option -e)
 *
 * inline-commands-prefix is an additional option that can be set
 * from gnutls-cli to change the default prefix (^) of inline commands.
 * This option is only relevant if inline-commands option is enabled. 
 * This option expects a single US-ASCII character (octets 0 - 127).
 * For ex: if --inline-commands-prefix=@, the inline commands will be 
 * @resume@, @renegotiate@, etc...
 */
typedef enum INLINE_COMMAND { INLINE_COMMAND_NONE,
	INLINE_COMMAND_RESUME,
	INLINE_COMMAND_RENEGOTIATE,
	INLINE_COMMAND_REKEY_LOCAL,
	INLINE_COMMAND_REKEY_BOTH
} inline_command_t;

#define MAX_INLINE_COMMAND_BYTES 20

typedef struct inline_cmds {
	char *current_ptr;	/* points to the start of the current buffer being processed */
	char *new_buffer_ptr;	/* points to start or offset within the caller's buffer,
				 * and refers to bytes yet to be processed. */
	inline_command_t cmd_found;
	int lf_found;
	int bytes_to_flush;
	ssize_t bytes_copied;
	char inline_cmd_buffer[MAX_INLINE_COMMAND_BYTES];
} inline_cmds_st;


struct inline_command_definitions {
	int command;
	char string[MAX_INLINE_COMMAND_BYTES];
};

/* All inline commands will contain a trailing LF */
struct inline_command_definitions inline_commands_def[] = {
	{INLINE_COMMAND_RESUME, "^resume^\n"},
	{INLINE_COMMAND_REKEY_LOCAL, "^rekey1^\n"},
	{INLINE_COMMAND_REKEY_BOTH, "^rekey^\n"},
	{INLINE_COMMAND_RENEGOTIATE, "^renegotiate^\n"},
};

#define NUM_INLINE_COMMANDS ((unsigned)(sizeof(inline_commands_def)/sizeof(inline_commands_def[0])))

#endif /* GNUTLS_SRC_INLINE_CMDS_H */
