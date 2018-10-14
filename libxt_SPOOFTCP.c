#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <xtables.h>

#include "xt_SPOOFTCP.h"

enum {
	O_TTL,
	O_TCP_FLAGS,
	O_CORRUPT_CHKSUM,
	O_CORRUPT_SEQ,
	O_DELAY,
	O_PAYLOAD_LEN,
};

/* Copied from libxt_tcp.c */
struct tcp_flag_names {
	const char *name;
	__u8 flag;
};

static const struct tcp_flag_names tcp_flag_names[]
= { { "FIN", 0x01 },
    { "SYN", 0x02 },
    { "RST", 0x04 },
    { "PSH", 0x08 },
    { "ACK", 0x10 },
    { "URG", 0x20 },
    { "ALL", 0x3F },
    { "NONE", 0 },
};
#define TCP_FLAG_NAMES_SIZE 8

static __u8 parse_tcp_flag(const char *flags)
{
	__u8 ret = 0;
	char *ptr;
	char *buffer;

	buffer = strdup(flags);

	for (ptr = strtok(buffer, ","); ptr; ptr = strtok(NULL, ",")) {
		unsigned int i;
		for (i = 0; i < TCP_FLAG_NAMES_SIZE; ++i)
			if (strcasecmp(tcp_flag_names[i].name, ptr) == 0) {
				ret |= tcp_flag_names[i].flag;
				break;
			}
		if (i == TCP_FLAG_NAMES_SIZE)
			xtables_error(PARAMETER_PROBLEM,
				   "Unknown TCP flag `%s'", ptr);
	}

	free(buffer);
	return ret;
}

static void print_tcpf(__u8 flags)
{
	int have_flag = 0;

	while (flags) {
		unsigned int i;

		for (i = 0; (flags & tcp_flag_names[i].flag) == 0; i++);

		if (have_flag)
			printf(",");
		printf("%s", tcp_flag_names[i].name);
		have_flag = 1;

		flags &= ~tcp_flag_names[i].flag;
	}

	if (!have_flag)
		printf("NONE");
}

static void SPOOFTCP_help()
{
	printf("SPOOFTCP target options:\n"
		" --ttl value\tThe hop limit/ttl value of spoofed packet (0 for inherit)\n"
		" --tcp-flags\tTCP FLAGS of spoofed packet\n"
		" --corrupt-checksum\tInvert checksum for spoofed packet\n"
		" --corrupt-seq\tInvert TCP SEQ # for spoofed packet\n"
		" --delay value\tDelay the matched(original) packet by <value> ms (max 255)\n"
		" --payload-length value\tLength of TCP payload (max 255)\n");
}

static const struct xt_option_entry SPOOFTCP_opts[] = {
	{
		.name	= "ttl",
		.id		= O_TTL,
		.type	= XTTYPE_UINT8,
		.min	= 0,
		.max	= UINT8_MAX,
		.flags	= XTOPT_PUT, XTOPT_POINTER(struct xt_spooftcp_info, ttl),
	},
	{
		.name	= "tcp-flags",
		.id		= O_TCP_FLAGS,
		.type	= XTTYPE_STRING,
	},
	{
		.name	= "corrupt-checksum",
		.id		= O_CORRUPT_CHKSUM,
		.type	= XTTYPE_NONE,
	},
	{
		.name	= "corrupt-seq",
		.id		= O_CORRUPT_SEQ,
		.type	= XTTYPE_NONE,
	},
	{
		.name	= "delay",
		.id		= O_DELAY,
		.type	= XTTYPE_UINT8,
		.min	= 0,
		.max	= UINT8_MAX,
		.flags	= XTOPT_PUT, XTOPT_POINTER(struct xt_spooftcp_info, delay),
	},
	{
		.name	= "payload-length",
		.id		= O_PAYLOAD_LEN,
		.type	= XTTYPE_UINT8,
		.min	= 0,
		.max	= UINT8_MAX,
		.flags	= XTOPT_PUT, XTOPT_POINTER(struct xt_spooftcp_info, payload_len),
	},
	XTOPT_TABLEEND,
};

static void SPOOFTCP_parse(struct xt_option_call *cb)
{
	struct xt_spooftcp_info *info = cb->data;
	const struct xt_option_entry *entry = cb->entry;

	xtables_option_parse(cb);

	switch(entry->id)
	{
		case O_TTL:
		case O_DELAY:
		case O_PAYLOAD_LEN:
			break; // Do nothing
		case O_TCP_FLAGS:
			info->tcp_flags = parse_tcp_flag(cb->arg);
			break;
		case O_CORRUPT_CHKSUM:
			info->corrupt_chksum = true;
			break;
		case O_CORRUPT_SEQ:
			info->corrupt_seq = true;
			break;
	}
}

static void SPOOFTCP_check(struct xt_fcheck_call *cb)
{
	if (!(cb->xflags & (1 << O_TCP_FLAGS)))
		xtables_error(PARAMETER_PROBLEM,
		           "SPOOFTCP target: --tcp-flags is required");
}

static void SPOOFTCP_print(const void *ip, const struct xt_entry_target *target,
                         int numeric)
{
	const struct xt_spooftcp_info *info =
		(const struct xt_spooftcp_info *)target->data;
	if (info->ttl)
		printf(" SPOOFTCP ttl = %u", info->ttl);
	else
		printf(" SPOOFTCP ttl inherit");

	printf(" tcp flags ");
	if (numeric)
		printf("0x%02X", info->tcp_flags);
	else
		print_tcpf(info->tcp_flags);

	if (info->corrupt_chksum)
		printf(" Corrupt checksum");

	if (info->corrupt_seq)
		printf(" Corrupt SEQ");

	if (info->delay)
		printf(" Delay by %ums", info->delay);

	if (info->payload_len)
		printf(" Payload length %u", info->payload_len);
}

static void SPOOFTCP_save(const void *ip, const struct xt_entry_target *target)
{
	const struct xt_spooftcp_info *info =
		(const struct xt_spooftcp_info *)target->data;

	if (info->ttl)
		printf(" --%s %u", SPOOFTCP_opts[O_TTL].name, info->ttl);

	printf(" --%s ", SPOOFTCP_opts[O_TCP_FLAGS].name);
	print_tcpf(info->tcp_flags);

	if (info->corrupt_chksum)
		printf(" --%s", SPOOFTCP_opts[O_CORRUPT_CHKSUM].name);

	if (info->corrupt_seq)
		printf(" --%s", SPOOFTCP_opts[O_CORRUPT_SEQ].name);

	if (info->delay)
		printf(" --%s %u", SPOOFTCP_opts[O_DELAY].name, info->delay);

	if (info->payload_len)
		printf(" --%s %u", SPOOFTCP_opts[O_PAYLOAD_LEN].name, info->payload_len);
}

static struct xtables_target spooftcp_tg_reg = {
	.family			= NFPROTO_UNSPEC,
	.name			= "SPOOFTCP",
	.version		= XTABLES_VERSION,
	.size			= XT_ALIGN(sizeof(struct xt_spooftcp_info)),
	.userspacesize	= XT_ALIGN(sizeof(struct xt_spooftcp_info)),
	.help			= SPOOFTCP_help,
	.print			= SPOOFTCP_print,
	.save			= SPOOFTCP_save,
	.x6_parse		= SPOOFTCP_parse,
	.x6_fcheck		= SPOOFTCP_check,
	.x6_options		= SPOOFTCP_opts,
};

void _init(void)
{
	xtables_register_target(&spooftcp_tg_reg);
}
