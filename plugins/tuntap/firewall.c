#include "common.h"

extern struct uwsgi_tuntap utt;

int uwsgi_tuntap_firewall_check(struct uwsgi_tuntap_firewall_rule *direction, char *pkt, uint16_t len) {
        // sanity check
        if (len < 20) return -1;
        uint32_t *src_ip = (uint32_t *) &pkt[12];
        uint32_t *dst_ip = (uint32_t *) &pkt[16];

        uint32_t src = *src_ip;
        uint32_t dst = *dst_ip;

        struct uwsgi_tuntap_firewall_rule *utfr = direction;
        while(utfr) {
                if (utfr->src) {
                        uint32_t src_masked = src & utfr->src_mask;
                        if (!src_masked != utfr->src) goto next;
                }

                if (utfr->dst) {
                        uint32_t dst_masked = dst & utfr->dst_mask;
                        if (!dst_masked != utfr->dst) goto next;
                }

                return utfr->action;
next:
                utfr = utfr->next;
        }

        return 0;
}

static void uwsgi_tuntap_firewall_add_rule(struct uwsgi_tuntap_firewall_rule *direction, uint8_t action, uint32_t src, uint32_t src_mask, uint32_t dst, uint32_t dst_mask) {
}

void uwsgi_tuntap_opt_firewall(char *opt, char *value, void *direction) {
	
	char *space = strchr(value, ' ');
	if (!space) {
		if (!strcmp("deny", value)) {
			uwsgi_tuntap_firewall_add_rule((struct uwsgi_tuntap_firewall_rule *) direction, 1, 0, 0, 0, 0);
			return;
		}
		uwsgi_tuntap_firewall_add_rule((struct uwsgi_tuntap_firewall_rule *) direction, 0, 0, 0, 0, 0);
		return;
	}

	*space = 0;

	uint8_t action = 0;
	if (!strcmp(value, "deny")) action = 1;

	char *space2 = strchr(space + 1, ' ');
	if (!space2) {
		uwsgi_log("invalid tuntap firewall rule syntax. must be <action> <src/mask> <dst/mask>");
	}

	*space2 = 0;

	uint32_t src = 0;
	uint32_t src_mask = 0;
	uint32_t dst = 0;
	uint32_t dst_mask = 0;

	char *slash = strchr(space + 1 , '/');
	if (slash) {
		src_mask = atoi(slash+1);
		*slash = 0;
	}

	if (inet_pton(AF_INET, space + 1, &src) != 1) {
		uwsgi_error("uwsgi_tuntap_opt_firewall()/inet_pton()");
		exit(1);
	}

	if (slash) *slash = '/';
	*space = ' ';

	slash = strchr(space2 + 1 , '/');
        if (slash) {
                dst_mask = atoi(slash+1);
                *slash = 0;
        }

        if (inet_pton(AF_INET, space2 + 1, &dst) != 1) {
                uwsgi_error("uwsgi_tuntap_opt_firewall()/inet_pton()");
                exit(1);
        }

        if (slash) *slash = '/';
        *space2 = ' ';

	uwsgi_tuntap_firewall_add_rule((struct uwsgi_tuntap_firewall_rule *) direction, action, src, 1 << src_mask, dst, 1 << dst_mask);
}
