// Read-only dumper for the Pico Neo 2 FPGA SPI NOR behind /dev/w25q.
//
// The driver performs real SPI fast-reads but returns 0 from read(), so dd/cat
// see EOF and write nothing. This ignores the return value and keeps whatever
// the driver deposited in the buffer.
//
// Never writes to the device. Opens O_RDONLY first and only escalates to O_RDWR
// if the driver refuses a read-only open.

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define POISON 0xA5

int main(int argc, char **argv) {
    size_t want = (argc > 1) ? strtoul(argv[1], NULL, 0) : 8192;
    const char *out = (argc > 2) ? argv[2] : "/data/local/tmp/w25q_dump.bin";

    int fd = open("/dev/w25q", O_RDONLY);
    int mode_used = 0;
    if (fd < 0) {
        fprintf(stderr, "O_RDONLY failed (%s), retrying O_RDWR\n", strerror(errno));
        fd = open("/dev/w25q", O_RDWR);
        mode_used = 1;
    }
    if (fd < 0) { fprintf(stderr, "open /dev/w25q failed: %s\n", strerror(errno)); return 1; }
    fprintf(stderr, "opened /dev/w25q (%s)\n", mode_used ? "O_RDWR" : "O_RDONLY");

    unsigned char *buf = malloc(want);
    if (!buf) { fprintf(stderr, "malloc %zu failed\n", want); return 1; }
    memset(buf, POISON, want);

    errno = 0;
    ssize_t r = read(fd, buf, want);
    fprintf(stderr, "read(%zu) returned %zd  errno=%d (%s)\n",
            want, r, errno, errno ? strerror(errno) : "none");

    // How much of the buffer did the driver actually touch?
    size_t touched = 0, ff = 0, zero = 0;
    for (size_t i = 0; i < want; i++) {
        if (buf[i] != POISON) touched++;
        if (buf[i] == 0xFF) ff++;
        if (buf[i] == 0x00) zero++;
    }
    fprintf(stderr, "buffer: %zu/%zu bytes changed from poison\n", touched, want);
    fprintf(stderr, "        0xFF=%zu  0x00=%zu  poison-left=%zu\n", ff, zero, want - touched);

    if (touched == 0) {
        fprintf(stderr, "VERDICT: driver did not fill the buffer at this size.\n");
    } else if (ff == want) {
        fprintf(stderr, "VERDICT: fully erased flash (all 0xFF).\n");
    } else if (touched == want) {
        fprintf(stderr, "VERDICT: full buffer of real data.\n");
    } else {
        fprintf(stderr, "VERDICT: partial fill - driver returned %zu bytes of data.\n", touched);
    }

    FILE *f = fopen(out, "wb");
    if (!f) { fprintf(stderr, "fopen %s failed: %s\n", out, strerror(errno)); close(fd); return 1; }
    fwrite(buf, 1, want, f);
    fclose(f);
    close(fd);
    fprintf(stderr, "wrote %s (%zu bytes)\n", out, want);
    return 0;
}
