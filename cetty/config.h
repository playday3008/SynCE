#define VARRUNDIR "/var/run"
#define MAXLINE 1024		/* max. # chars in a line */
#define MAXPATH MAXLINE

/* defines for auto detection of incoming PPP calls (->PAP/CHAP) */

#define PPP_FRAME	0x7e	/* PPP Framing character */
#define PPP_STATION	0xff	/* "All Station" character */
#define PPP_ESCAPE	0x7d	/* Escape Character */ 
#define PPP_CONTROL	0x03	/* PPP Control Field */
#define PPP_LCP_HI	0xc0	/* LCP protocol - high byte */
#define PPP_LCP_LOW	0x21	/* LCP protocol - low byte */
#define PPP_UNESCAPE(c)	((c) ^ 0x20) /* un-escape character */

