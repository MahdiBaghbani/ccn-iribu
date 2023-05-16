/*
 * @f util/ccn-iribu-mkI.c
 * @b CLI mkInterest, write to Stdout
 *
 * Copyright (C) 2013-18, Christian Tschudin, University of Basel
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "ccn-iribu-common.h"

// ----------------------------------------------------------------------

int main(int argc, char *argv[])
{

    char *digest = 0, *publisher = 0;
    char *fname = 0;
    int f, opt;
    int dlen = 0, plen = 0;
    int packettype = CCN_IRIBU_SUITE_NDNTLV;
    struct ccn_iribu_prefix_s *prefix;
    time_t curtime;
    uint32_t nonce;
    int isLambda          = 0;
    unsigned int chunknum = UINT_MAX;
    ccn_iribu_interest_opts_u int_opts;

    time(&curtime);
    // Get current time in double to avoid dealing with time_t
    nonce = (uint32_t) difftime(curtime, 0);

    while ((opt = getopt(argc, argv, "hd:e:i:ln:o:p:s:v:")) != -1) {
        switch (opt) {
        case 'd':
            digest = optarg;
            dlen   = unescape_component(digest);
            if (dlen != 32) {
                DEBUGMSG(ERROR, "digest has wrong length (%d instead of 32)\n", dlen);
                exit(-1);
            }
            break;
        case 'e':
            nonce = (int) strtol(optarg, (char **) NULL, 10);
            break;
        case 'l':
            isLambda = 1 - isLambda;
            break;
        case 'n':
            chunknum = (int) strtol(optarg, (char **) NULL, 10);
            break;
        case 'o':
            fname = optarg;
            break;
        case 'p':
            publisher = optarg;
            plen      = unescape_component(publisher);
            if (plen != 32) {
                DEBUGMSG(ERROR,
                         "publisher key digest has wrong length (%d instead of 32)\n",
                         plen);
                exit(-1);
            }
            break;
        case 'v':
#ifdef USE_LOGGING
            if (isdigit(optarg[0]))
                debug_level = (int) strtol(optarg, (char **) NULL, 10);
            else
                debug_level = ccn_iribu_debug_str2level(optarg);
#endif
            break;
        case 's':
            packettype = ccn_iribu_str2suite(optarg);
            if (packettype >= 0 && packettype < CCN_IRIBU_SUITE_LAST)
                break;
        /* falls through */
        case 'h':
        /* falls through */
        default:
        Usage:
            fprintf(
                stderr,
                "usage: %s [options] URI\n"
                "  -d DIGEST  content digest (sets -x to 0)\n"
                "  -e NONCE   random 4 bytes\n"
                "  -l         URI is a Lambda expression\n"
                "  -n CHUNKNUM positive integer for chunk interest\n"
                "  -o FNAME   output file (instead of stdout)\n"
                "  -p DIGEST  publisher fingerprint\n"
#ifdef USE_LOGGING
                "  -v DEBUG_LEVEL (fatal, error, warning, info, debug, verbose, trace)\n"
#endif
                "  -s SUITE   (ccnb, ccnx2015, ndn2013)\n",
                argv[0]);
            exit(1);
        }
        /* falls through */
    }

    if (!argv[optind])
        goto Usage;

    /*
    if (isLambda)
        i = ccn_iribu_lambdaStrToComponents(prefix, argv[optind]);
    else
    */
    prefix = ccn_iribu_URItoPrefix(argv[optind], packettype,
                                   chunknum == UINT_MAX ? NULL : &chunknum);
    if (!prefix) {
        DEBUGMSG(ERROR, "no URI found, aborting\n");
        return -1;
    }

    prefix->suite = packettype;
#ifdef USE_SUITE_NDNTLV
    int_opts.ndntlv.nonce = nonce;
#endif
    struct ccn_iribu_buf_s *buf = ccn_iribu_mkSimpleInterest(prefix, &int_opts);

    if (buf->datalen <= 0) {
        DEBUGMSG(ERROR, "internal error: empty packet\n");
        return -1;
    }

    if (fname) {
        f = creat(fname, 0666);
        if (f < 0) {
            perror("file open:");
            return -1;
        }
    } else
        f = 1;

    write(f, buf->data, buf->datalen);
    close(f);

    return 0;
}

// eof