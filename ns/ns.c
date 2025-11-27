/*
* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 *
 * Network debugger
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <argparser.h>
#include <fx/typedefs.h>

/* --flushdns */
static int run_flush(void)
{
        int r;

#ifdef _WIN32
        r = system("ipconfig /flushdns");
#elif __linux__
        r = system("sudo systemd-resolve --flush-caches");
#elif __APPLE__
        r = system("sudo dscacheutil -flushcache; sudo killall -HUP mDNSResponder 2>/dev/null");
        if (r != 0)
                r = system("sudo dscacheutil -flushcache; sudo killall -HUP mDNSResponder 2>/dev/null");
        if (r != 0)
                r = system("sudo killall -HUP mDNSResponder 2>/dev/null");
#endif
        if (r == 0)
                printf("DNS cache flushed successfully.\n");
        else
                printf("Failed to flush DNS cache.\n");

        exit(0);
}

static int run_dns(void)
{
        struct __res_state res;

        if (res_ninit(&res) != 0)
                die("res_ninit failed");

        for (int i = 0; i < res.nscount; i++)
                printf("%s\n", inet_ntoa(res.nsaddr_list[i].sin_addr));

        res_nclose(&res);

        exit(0);
}

static void run_resolv(void)
{
        char buf[4096];
        FILE *fp;

        fp = fopen("/etc/resolv.conf", "r");
        if (!fp) {
                perror("cannot open /etc/resolv.conf");
                exit(1);
        }

        while (fgets(buf, sizeof(buf), fp))
                fputs(buf, stdout);

        fclose(fp);
        exit(0);
}

int main(int argc, char **argv)
{
        struct argparser *ap;
        struct option *opt_dns;
        struct option *opt_resolv;
        struct option *opt_flush_dns;

        ap = argparser_create();
        if (!ap)
                return -1;

        argparser_add1(ap, &opt_dns, "dns", NULL, "show resolv DNS address and exit", OP_NULL);
        argparser_add0(ap, &opt_resolv, "resolv", NULL, "show resolv.conf and exit", OP_NULL);
        argparser_add0(ap, &opt_flush_dns, NULL, "flushdns", "flush system DNS cache and exit", OP_NULL);

        if (argparser_run(ap, argc, argv) != 0) {
                fprintf(stderr, "%s\n", argparser_error(ap));
                argparser_free(ap);
                return -1;
        }

        if (opt_dns)
                run_dns();

        if (opt_resolv)
                run_resolv();

        if (opt_flush_dns)
                run_flush();

        return 0;
}