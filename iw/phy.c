#include <stdbool.h>
#include <errno.h>
#include <strings.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>

#include "nl80211.h"
#include "iw.h"

struct channels_ctx {
	int last_band;
	bool width_40;
	bool width_80;
	bool width_160;
};

static char *dfs_state_name(enum nl80211_dfs_state state)
{
	switch (state) {
	case NL80211_DFS_USABLE:
		return "usable";
	case NL80211_DFS_AVAILABLE:
		return "available";
	case NL80211_DFS_UNAVAILABLE:
		return "unavailable";
	default:
		return "unknown";
	}
}

static int print_channels_handler(struct nl_msg *msg, void *arg)
{
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct channels_ctx *ctx = arg;
	struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];
	struct nlattr *tb_band[NL80211_BAND_ATTR_MAX + 1];
	struct nlattr *tb_freq[NL80211_FREQUENCY_ATTR_MAX + 1];
	struct nlattr *nl_band;
	struct nlattr *nl_freq;
	int rem_band, rem_freq;

	nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

	if (tb_msg[NL80211_ATTR_WIPHY_BANDS]) {
		nla_for_each_nested(nl_band, tb_msg[NL80211_ATTR_WIPHY_BANDS], rem_band) {
			if (ctx->last_band != nl_band->nla_type) {
				printf("Band %d:\n", nl_band->nla_type + 1);
				ctx->width_40 = false;
				ctx->width_80 = false;
				ctx->width_160 = false;
				ctx->last_band = nl_band->nla_type;
			}

			nla_parse(tb_band, NL80211_BAND_ATTR_MAX, nla_data(nl_band), nla_len(nl_band), NULL);

			if (tb_band[NL80211_BAND_ATTR_HT_CAPA]) {
				__u16 cap = nla_get_u16(tb_band[NL80211_BAND_ATTR_HT_CAPA]);

				if (cap & BIT(1))
					ctx->width_40 = true;
			}

			if (tb_band[NL80211_BAND_ATTR_VHT_CAPA]) {
				__u32 capa;

				ctx->width_80 = true;

				capa = nla_get_u32(tb_band[NL80211_BAND_ATTR_VHT_CAPA]);
				switch ((capa >> 2) & 3) {
				case 2:
					/* width_80p80 = true; */
					/* fall through */
				case 1:
					ctx->width_160 = true;
				break;
				}
			}

			if (tb_band[NL80211_BAND_ATTR_FREQS]) {
				nla_for_each_nested(nl_freq, tb_band[NL80211_BAND_ATTR_FREQS], rem_freq) {
					uint32_t freq;

					nla_parse(tb_freq, NL80211_FREQUENCY_ATTR_MAX, nla_data(nl_freq), nla_len(nl_freq), NULL);

					if (!tb_freq[NL80211_FREQUENCY_ATTR_FREQ])
						continue;
					freq = nla_get_u32(tb_freq[NL80211_FREQUENCY_ATTR_FREQ]);
					printf("\t* %d MHz [%d] ", freq, ieee80211_frequency_to_channel(freq));

					if (tb_freq[NL80211_FREQUENCY_ATTR_DISABLED]) {
						printf("(disabled)\n");
						continue;
					}
					printf("\n");

					if (tb_freq[NL80211_FREQUENCY_ATTR_MAX_TX_POWER])
						printf("\t  Maximum TX power: %.1f dBm\n", 0.01 * nla_get_u32(tb_freq[NL80211_FREQUENCY_ATTR_MAX_TX_POWER]));

					/* If both flags are set assume an new kernel */
					if (tb_freq[NL80211_FREQUENCY_ATTR_NO_IR] && tb_freq[__NL80211_FREQUENCY_ATTR_NO_IBSS]) {
						printf("\t  No IR\n");
					} else if (tb_freq[NL80211_FREQUENCY_ATTR_PASSIVE_SCAN]) {
						printf("\t  Passive scan\n");
					} else if (tb_freq[__NL80211_FREQUENCY_ATTR_NO_IBSS]){
						printf("\t  No IBSS\n");
					}

					if (tb_freq[NL80211_FREQUENCY_ATTR_RADAR])
						printf("\t  Radar detection\n");

					printf("\t  Channel widths:");
					if (!tb_freq[NL80211_FREQUENCY_ATTR_NO_20MHZ])
						printf(" 20MHz");
					if (ctx->width_40 && !tb_freq[NL80211_FREQUENCY_ATTR_NO_HT40_MINUS])
						printf(" HT40-");
					if (ctx->width_40 && !tb_freq[NL80211_FREQUENCY_ATTR_NO_HT40_PLUS])
						printf(" HT40+");
					if (ctx->width_80 && !tb_freq[NL80211_FREQUENCY_ATTR_NO_80MHZ])
						printf(" VHT80");
					if (ctx->width_160 && !tb_freq[NL80211_FREQUENCY_ATTR_NO_160MHZ])
						printf(" VHT160");
					printf("\n");

					if (!tb_freq[NL80211_FREQUENCY_ATTR_DISABLED] && tb_freq[NL80211_FREQUENCY_ATTR_DFS_STATE]) {
						enum nl80211_dfs_state state = nla_get_u32(tb_freq[NL80211_FREQUENCY_ATTR_DFS_STATE]);
						unsigned long time;

						printf("\t  DFS state: %s", dfs_state_name(state));
						if (tb_freq[NL80211_FREQUENCY_ATTR_DFS_TIME]) {
							time = nla_get_u32(tb_freq[NL80211_FREQUENCY_ATTR_DFS_TIME]);
							printf(" (for %lu sec)", time / 1000);
						}
						printf("\n");
						if (tb_freq[NL80211_FREQUENCY_ATTR_DFS_CAC_TIME])
							printf("\t  DFS CAC time: %u ms\n",
							       nla_get_u32(tb_freq[NL80211_FREQUENCY_ATTR_DFS_CAC_TIME]));
					}
				}
			}
		}
	}

	return NL_SKIP;
}

static int handle_channels(struct nl80211_state *state, struct nl_msg *msg,
			   int argc, char **argv, enum id_input id)
{
	static struct channels_ctx ctx = {
		.last_band = -1,
	};

	nla_put_flag(msg, NL80211_ATTR_SPLIT_WIPHY_DUMP);
	nlmsg_hdr(msg)->nlmsg_flags |= NLM_F_DUMP;

	register_handler(print_channels_handler, &ctx);

	return 0;
}
TOPLEVEL(channels, NULL, NL80211_CMD_GET_WIPHY, 0, CIB_PHY, handle_channels, "Show available channels.");

static int handle_name(struct nl80211_state *state,
		       struct nl_msg *msg,
		       int argc, char **argv,
		       enum id_input id)
{
	if (argc != 1)
		return 1;

	NLA_PUT_STRING(msg, NL80211_ATTR_WIPHY_NAME, *argv);

	return 0;
 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(set, name, "<new name>", NL80211_CMD_SET_WIPHY, 0, CIB_PHY, handle_name,
	"Rename this wireless device.");

static int handle_freqs(struct nl_msg *msg, int argc, char **argv)
{
	static const struct {
		const char *name;
		unsigned int val;
	} bwmap[] = {
		{ .name = "20", .val = NL80211_CHAN_WIDTH_20, },
		{ .name = "40", .val = NL80211_CHAN_WIDTH_40, },
		{ .name = "80", .val = NL80211_CHAN_WIDTH_80, },
		{ .name = "80+80", .val = NL80211_CHAN_WIDTH_80P80, },
		{ .name = "160", .val = NL80211_CHAN_WIDTH_160, },
	};
	uint32_t freq;
	unsigned int i, bwval = NL80211_CHAN_WIDTH_20_NOHT;
	char *end;

	if (argc < 1)
		return 1;

	for (i = 0; i < ARRAY_SIZE(bwmap); i++) {
		if (strcasecmp(bwmap[i].name, argv[0]) == 0) {
			bwval = bwmap[i].val;
			break;
		}
	}

	if (bwval == NL80211_CHAN_WIDTH_20_NOHT)
		return 1;

	NLA_PUT_U32(msg, NL80211_ATTR_CHANNEL_WIDTH, bwval);

	if (argc == 1)
		return 0;

	/* center freq 1 */
	if (!*argv[1])
		return 1;
	freq = strtoul(argv[1], &end, 10);
	if (*end)
		return 1;
	NLA_PUT_U32(msg, NL80211_ATTR_CENTER_FREQ1, freq);

	if (argc == 2)
		return 0;

	/* center freq 2 */
	if (!*argv[2])
		return 1;
	freq = strtoul(argv[2], &end, 10);
	if (*end)
		return 1;
	NLA_PUT_U32(msg, NL80211_ATTR_CENTER_FREQ2, freq);

	return 0;
 nla_put_failure:
	return -ENOBUFS;
}

static int handle_freqchan(struct nl_msg *msg, bool chan,
			   int argc, char **argv)
{
	char *end;
	static const struct {
		const char *name;
		unsigned int val;
	} htmap[] = {
		{ .name = "HT20", .val = NL80211_CHAN_HT20, },
		{ .name = "HT40+", .val = NL80211_CHAN_HT40PLUS, },
		{ .name = "HT40-", .val = NL80211_CHAN_HT40MINUS, },
	};
	unsigned int htval = NL80211_CHAN_NO_HT;
	unsigned int freq;
	unsigned int i;

	if (!argc || argc > 4)
		return 1;

	if (!*argv[0])
		return 1;
	freq = strtoul(argv[0], &end, 10);
	if (*end)
		return 1;

	if (chan) {
		enum nl80211_band band;
		band = freq <= 14 ? NL80211_BAND_2GHZ : NL80211_BAND_5GHZ;
		freq = ieee80211_channel_to_frequency(freq, band);
	}

	NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_FREQ, freq);

	if (argc > 2) {
		return handle_freqs(msg, argc - 1, argv + 1);
	} else if (argc == 2) {
		for (i = 0; i < ARRAY_SIZE(htmap); i++) {
			if (strcasecmp(htmap[i].name, argv[1]) == 0) {
				htval = htmap[i].val;
				break;
			}
		}
		if (htval == NL80211_CHAN_NO_HT)
			return handle_freqs(msg, argc - 1, argv + 1);
	}

	NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_CHANNEL_TYPE, htval);

	return 0;
 nla_put_failure:
	return -ENOBUFS;
}

static int handle_freq(struct nl80211_state *state, struct nl_msg *msg,
		       int argc, char **argv,
		       enum id_input id)
{
	return handle_freqchan(msg, false, argc, argv);
}
COMMAND(set, freq, "<freq> [HT20|HT40+|HT40-]",
	NL80211_CMD_SET_WIPHY, 0, CIB_PHY, handle_freq,
	"Set frequency/channel the hardware is using, including HT\n"
	"configuration.");
COMMAND(set, freq, "<freq> [HT20|HT40+|HT40-]\n"
		   "<control freq> [20|40|80|80+80|160] [<center freq 1>] [<center freq 2>]",
	NL80211_CMD_SET_WIPHY, 0, CIB_NETDEV, handle_freq, NULL);

static int handle_chan(struct nl80211_state *state, struct nl_msg *msg,
		       int argc, char **argv,
		       enum id_input id)
{
	return handle_freqchan(msg, true, argc, argv);
}
COMMAND(set, channel, "<channel> [HT20|HT40+|HT40-]",
	NL80211_CMD_SET_WIPHY, 0, CIB_PHY, handle_chan, NULL);
COMMAND(set, channel, "<channel> [HT20|HT40+|HT40-]",
	NL80211_CMD_SET_WIPHY, 0, CIB_NETDEV, handle_chan, NULL);

static int handle_fragmentation(struct nl80211_state *state,
				struct nl_msg *msg,
				int argc, char **argv,
				enum id_input id)
{
	unsigned int frag;

	if (argc != 1)
		return 1;

	if (strcmp("off", argv[0]) == 0)
		frag = -1;
	else {
		char *end;

		if (!*argv[0])
			return 1;
		frag = strtoul(argv[0], &end, 10);
		if (*end != '\0')
			return 1;
	}

	NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_FRAG_THRESHOLD, frag);

	return 0;
 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(set, frag, "<fragmentation threshold|off>",
	NL80211_CMD_SET_WIPHY, 0, CIB_PHY, handle_fragmentation,
	"Set fragmentation threshold.");

static int handle_rts(struct nl80211_state *state,
		      struct nl_msg *msg,
		      int argc, char **argv,
		      enum id_input id)
{
	unsigned int rts;

	if (argc != 1)
		return 1;

	if (strcmp("off", argv[0]) == 0)
		rts = -1;
	else {
		char *end;

		if (!*argv[0])
			return 1;
		rts = strtoul(argv[0], &end, 10);
		if (*end != '\0')
			return 1;
	}

	NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_RTS_THRESHOLD, rts);

	return 0;
 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(set, rts, "<rts threshold|off>",
	NL80211_CMD_SET_WIPHY, 0, CIB_PHY, handle_rts,
	"Set rts threshold.");

static int handle_retry(struct nl80211_state *state,
			struct nl_msg *msg,
			int argc, char **argv, enum id_input id)
{
	unsigned int retry_short = 0, retry_long = 0;
	bool have_retry_s = false, have_retry_l = false;
	int i;
	enum {
		S_NONE,
		S_SHORT,
		S_LONG,
	} parser_state = S_NONE;

	if (!argc || (argc != 2 && argc != 4))
		return 1;

	for (i = 0; i < argc; i++) {
		char *end;
		unsigned int tmpul;

		if (strcmp(argv[i], "short") == 0) {
			if (have_retry_s)
				return 1;
			parser_state = S_SHORT;
			have_retry_s = true;
		} else if (strcmp(argv[i], "long") == 0) {
			if (have_retry_l)
				return 1;
			parser_state = S_LONG;
			have_retry_l = true;
		} else {
			tmpul = strtoul(argv[i], &end, 10);
			if (*end != '\0')
				return 1;
			if (!tmpul || tmpul > 255)
				return -EINVAL;
			switch (parser_state) {
			case S_SHORT:
				retry_short = tmpul;
				break;
			case S_LONG:
				retry_long = tmpul;
				break;
			default:
				return 1;
			}
		}
	}

	if (!have_retry_s && !have_retry_l)
		return 1;
	if (have_retry_s)
		NLA_PUT_U8(msg, NL80211_ATTR_WIPHY_RETRY_SHORT, retry_short);
	if (have_retry_l)
		NLA_PUT_U8(msg, NL80211_ATTR_WIPHY_RETRY_LONG, retry_long);

	return 0;
 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(set, retry, "[short <limit>] [long <limit>]",
	NL80211_CMD_SET_WIPHY, 0, CIB_PHY, handle_retry,
	"Set retry limit.");

#ifndef NETNS_RUN_DIR
#define NETNS_RUN_DIR "/var/run/netns"
#endif
static int netns_get_fd(const char *name)
{
	char pathbuf[MAXPATHLEN];
	const char *path, *ptr;

	path = name;
	ptr = strchr(name, '/');
	if (!ptr) {
		snprintf(pathbuf, sizeof(pathbuf), "%s/%s",
			NETNS_RUN_DIR, name );
		path = pathbuf;
	}
	return open(path, O_RDONLY);
}

static int handle_netns(struct nl80211_state *state,
			struct nl_msg *msg,
			int argc, char **argv,
			enum id_input id)
{
	char *end;
	int fd;

	if (argc < 1 || !*argv[0])
		return 1;

	if (argc == 1) {
		NLA_PUT_U32(msg, NL80211_ATTR_PID,
				strtoul(argv[0], &end, 10));
		if (*end != '\0') {
			printf("Invalid parameter: pid(%s)\n", argv[0]);
			return 1;
		}
		return 0;
	}

	if (argc != 2 || strcmp(argv[0], "name"))
		return 1;

	if ((fd = netns_get_fd(argv[1])) >= 0) {
		NLA_PUT_U32(msg, NL80211_ATTR_NETNS_FD, fd);
		return 0;
	} else {
		printf("Invalid parameter: nsname(%s)\n", argv[0]);
	}

	return 1;

 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(set, netns, "{ <pid> | name <nsname> }",
	NL80211_CMD_SET_WIPHY_NETNS, 0, CIB_PHY, handle_netns,
	"Put this wireless device into a different network namespace:\n"
	"    <pid>    - change network namespace by process id\n"
	"    <nsname> - change network namespace by name from "NETNS_RUN_DIR"\n"
	"               or by absolute path (man ip-netns)\n");

static int handle_coverage(struct nl80211_state *state,
			struct nl_msg *msg,
			int argc, char **argv,
			enum id_input id)
{
	char *end;
	unsigned int coverage;

	if (argc != 1)
		return 1;

	if (!*argv[0])
		return 1;
	coverage = strtoul(argv[0], &end, 10);
	if (coverage > 255)
		return 1;

	if (*end)
		return 1;

	NLA_PUT_U8(msg, NL80211_ATTR_WIPHY_COVERAGE_CLASS, coverage);

	return 0;
 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(set, coverage, "<coverage class>",
	NL80211_CMD_SET_WIPHY, 0, CIB_PHY, handle_coverage,
	"Set coverage class (1 for every 3 usec of air propagation time).\n"
	"Valid values: 0 - 255.");

static int handle_distance(struct nl80211_state *state,
			struct nl_msg *msg,
			int argc, char **argv,
			enum id_input id)
{
	if (argc != 1)
		return 1;

	if (!*argv[0])
		return 1;

	if (strcmp("auto", argv[0]) == 0) {
		NLA_PUT_FLAG(msg, NL80211_ATTR_WIPHY_DYN_ACK);
	} else {
		char *end;
		unsigned int distance, coverage;

		distance = strtoul(argv[0], &end, 10);

		if (*end)
			return 1;

		/*
		 * Divide double the distance by the speed of light
		 * in m/usec (300) to get round-trip time in microseconds
		 * and then divide the result by three to get coverage class
		 * as specified in IEEE 802.11-2007 table 7-27.
		 * Values are rounded upwards.
		 */
		coverage = (distance + 449) / 450;
		if (coverage > 255)
			return 1;

		NLA_PUT_U8(msg, NL80211_ATTR_WIPHY_COVERAGE_CLASS, coverage);
	}

	return 0;
 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(set, distance, "<auto|distance>",
	NL80211_CMD_SET_WIPHY, 0, CIB_PHY, handle_distance,
	"Enable ACK timeout estimation algorithm (dynack) or set appropriate\n"
	"coverage class for given link distance in meters.\n"
	"To disable dynack set valid value for coverage class.\n"
	"Valid values: 0 - 114750");

static int handle_txpower(struct nl80211_state *state,
			  struct nl_msg *msg,
			  int argc, char **argv,
			  enum id_input id)
{
	enum nl80211_tx_power_setting type;
	int mbm;

	/* get the required args */
	if (argc != 1 && argc != 2)
		return 1;

	if (!strcmp(argv[0], "auto"))
		type = NL80211_TX_POWER_AUTOMATIC;
	else if (!strcmp(argv[0], "fixed"))
		type = NL80211_TX_POWER_FIXED;
	else if (!strcmp(argv[0], "limit"))
		type = NL80211_TX_POWER_LIMITED;
	else {
		printf("Invalid parameter: %s\n", argv[0]);
		return 2;
	}

	NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_TX_POWER_SETTING, type);

	if (type != NL80211_TX_POWER_AUTOMATIC) {
		char *endptr;
		if (argc != 2) {
			printf("Missing TX power level argument.\n");
			return 2;
		}

		mbm = strtol(argv[1], &endptr, 10);
		if (*endptr)
			return 2;
		NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_TX_POWER_LEVEL, mbm);
	} else if (argc != 1)
		return 1;

	return 0;

 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(set, txpower, "<auto|fixed|limit> [<tx power in mBm>]",
	NL80211_CMD_SET_WIPHY, 0, CIB_PHY, handle_txpower,
	"Specify transmit power level and setting type.");
COMMAND(set, txpower, "<auto|fixed|limit> [<tx power in mBm>]",
	NL80211_CMD_SET_WIPHY, 0, CIB_NETDEV, handle_txpower,
	"Specify transmit power level and setting type.");

static int handle_antenna(struct nl80211_state *state,
			  struct nl_msg *msg,
			  int argc, char **argv,
			  enum id_input id)
{
	char *end;
	uint32_t tx_ant = 0, rx_ant = 0;

	if (argc == 1 && strcmp(argv[0], "all") == 0) {
		tx_ant = 0xffffffff;
		rx_ant = 0xffffffff;
	} else if (argc == 1) {
		tx_ant = rx_ant = strtoul(argv[0], &end, 0);
		if (*end)
			return 1;
	}
	else if (argc == 2) {
		tx_ant = strtoul(argv[0], &end, 0);
		if (*end)
			return 1;
		rx_ant = strtoul(argv[1], &end, 0);
		if (*end)
			return 1;
	} else
		return 1;

	NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_ANTENNA_TX, tx_ant);
	NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_ANTENNA_RX, rx_ant);

	return 0;

 nla_put_failure:
	return -ENOBUFS;
}
COMMAND(set, antenna, "<bitmap> | all | <tx bitmap> <rx bitmap>",
	NL80211_CMD_SET_WIPHY, 0, CIB_PHY, handle_antenna,
	"Set a bitmap of allowed antennas to use for TX and RX.\n"
	"The driver may reject antenna configurations it cannot support.");
