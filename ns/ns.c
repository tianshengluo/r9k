/*
* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 *
 * Network debugger
 */
#include <stdio.h>
#include <stdlib.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <argparser.h>
#include <fx/ioutils.h>

static void show_dns_list()
{
        struct __res_state res;

        if (res_ninit(&res) != 0) {
                perror("res_ninit failed");
                exit(1);
        }

        for (int i = 0; i < res.nscount; i++)
                printf("%s\n", inet_ntoa(res.nsaddr_list[i].sin_addr));

        res_nclose(&res);
        exit(0);
}

static void show_resolv_conf()
{
        char *buf = freadbuf("/etc/resolv.conf");
        if (!buf)
                exit(1);

        printf("%s", buf);
        free(buf);
}

int main(int argc, char **argv)
{
        struct argparser *ap;
        struct option *dns;
        struct option *resolv;

        ap = argparser_create();
        if (!ap)
                return -1;

        argparser_add0(ap, &dns, "dns", NULL, "show resolv DNS address and exit", OP_NULL);
        argparser_add0(ap, &resolv, NULL, "resolv", "show resolv.conf and exit", OP_NULL);

        if (argparser_run(ap, argc, argv) != 0) {
                fprintf(stderr, "%s\n", argparser_error(ap));
                argparser_free(ap);
                return -1;
        }

        if (dns) show_dns_list();
        if (resolv) show_resolv_conf();

        return 0;
}
