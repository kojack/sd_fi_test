/* sd_fi_test.c */
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/scsi/scsi_types.h>
#include <sys/dditypes.h>
#include <sys/cred.h>
#include <sys/scsi/scsi_pkt.h>

#define	SDIOC		('T'<<8)
#define	SDIOCSTART	(SDIOC|1)
#define	SDIOCSTOP	(SDIOC|2)
#define	SDIOCINSERTPKT	(SDIOC|3)
#define	SDIOCINSERTXB	(SDIOC|4)
#define	SDIOCINSERTUN	(SDIOC|5)
#define	SDIOCINSERTARQ	(SDIOC|6)
#define	SDIOCPUSH	(SDIOC|7)
#define	SDIOCRETRIEVE	(SDIOC|8)
#define	SDIOCRUN	(SDIOC|9)

/*
 * struct sd_fi_pkt definition.
 */
struct sd_fi_pkt {
	uint_t  pkt_flags;	/* flags */
	uchar_t pkt_scbp;	/* pointer to status block */
	uchar_t pkt_cdbp;	/* pointer to command block */
	uint_t  pkt_state;	/* state of command */
	uint_t  pkt_statistics;	/* statistics */
	uchar_t pkt_reason;	/* reason completion called */
};

/*
 * Define a transport error condition.
 */
struct sd_fi_pkt t_pkt_tran_err = {
	0,		/* pkt_flags */
	0,		/* pkt_scbp */
	0xff,		/* pkt_cdbp, 0xff keeps the original value unchanged. */
	STATE_GOT_BUS,	/* pkt_state */
	STAT_TIMEOUT,	/* pkt_statistics */
	CMD_TRAN_ERR	/* pkt_reason */
};

int main(int argc, char **argv) {
	int	fd = 0;
	unsigned char buf[512];

	if (argc < 2) {
		printf("./test_fault_injection <devpath>\n");
		return (0);
	}

	/*
	 * Start fault injection.
	 */
	if ((fd = open(argv[1], O_RDONLY|O_NDELAY)) == -1)
		perror("open");
	if (ioctl(fd, SDIOCSTART, NULL) == -1)
		perror("ioctl SDIOCSTART");

	/*
	 * Inject a transport error.
	 */
	if (ioctl(fd, SDIOCINSERTPKT, &t_pkt_tran_err) == -1)
		perror("ioctl SDIOCINSERTPKT");
	if (ioctl(fd, SDIOCPUSH, NULL) == -1)
		perror("ioctl SDIOCPUSH");

	/*
	 * Send out a SCSI command (READ).
	 */
	if (ioctl(fd, SDIOCRUN, NULL) == -1)
		perror("ioctl SDIOCRUN");
	if (read(fd, buf, sizeof (buf)) < 0)
		perror("read");

	/*
	 * Stop fault injection.
	 */
	if (ioctl(fd, SDIOCSTOP, NULL) == -1)
		perror("ioctl SDIOCSTOP");

	return (0);
}
