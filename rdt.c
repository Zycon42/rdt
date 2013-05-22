/**
 * Projekt c.3 do predmetu IPK
 * @author Jan Dusek <xdusek17@stud.fit.vutbr.cz>
 * @date 18.4.2011
 */

#include "rdt.h"
#include "udt.h"
#include "checksum.h"
#include "queue.h"

#include <sys/time.h>
#include <string.h>
#include <signal.h>

/// packet header size
#define HEADER_LEN 4
/// size of non data packet
#define NON_DATA_PACKET_LEN HEADER_LEN
/// max packet data size
#define MAX_DATA_LEN 80
/// max packet size
#define MAX_PACKET_LEN (80 + HEADER_LEN)
/// size of sender sliding window
#define WINDOW_SIZE 10
#define SEQUENCE_SIZE (3 * WINDOW_SIZE)
/// packet timeout in microseconds
#define TIMEOUT 500000

static const in_addr_t REMOTE_ADDR = 0x7f000001;

int alarm_arrived = 0;

/// RDT Header control codes
enum cnt_codes {
    /// Data sending
    C_DAT = 0,
    /// Establish connection
    C_EST = 1,
    /// Close connection
    C_CLOSE = 2,
    /// Data acknowledge
    C_DACK = 3,
    /// Establish acknowledge
    C_EACK = 4,
};

/// Connection states
enum states {
    /// No connection. Init state
    S_CLOSED,
    S_ESTABLISHED,
};

struct _rdt {
    /// descriptor for udt
    int udt;
    /// connection state
    enum states state;
    /// port which is rdt connected
    in_port_t remote_port;
};

/// rdt packet
typedef struct {
    /// packet number in sliding window
    int number;
    /// control code from enum cnt_codes
    int cnt_code;
    /// packet data. NOT OWNER
    void *data;
    /// size of packet data
    size_t datalen;
} packet_t;

typedef struct {
    void *data;
    size_t n;
} pack_data_t;

/******************************************************/
/*************** STATIC FUNCTIONS *********************/
/******************************************************/

/**
 * Creates packet. Stores packet to buf.
 * @param buf buffer to store packet.
 * @param buflen size of buffer
 * @param ipacket packet number in window. only first 13bits used
 * @param cnt_code type of packet from enum cnt_codes
 * @param data buffer where data is stored. This parameter only takes place in DAT packet
 * @param nbytes size of data buffer. On non DAT packets must be 0
 * @return size of created packet or 0 on error
 */
int create_packet(void *buf, size_t buflen, int ipacket, int cnt_code, void *data, size_t nbytes) {
    // required buflen is datalen + headerlen(4)
    if (buflen < nbytes + HEADER_LEN)
        return 0;

    // create 16bit field with 13bit pack_num and 3bit cnt_code
    uint16_t pn_cnt = ((ipacket & 0x1fff) << 3) | (cnt_code & 0x7);
    uint16_t *buf16 = (uint16_t *)buf;
    buf16[1] = pn_cnt;    // store it to second header word

    // if we have data copy them to packet after header
    if (data != NULL)
        memcpy((char *)buf + HEADER_LEN, data, nbytes);

    // compute checksum starting from second header word to data end
    uint16_t checksum = compute_checksum((char *)buf + 2, nbytes + 2);
    buf16[0] = checksum;  // store it to first header word

    return nbytes + HEADER_LEN;     // we have written header + data
}

/**
 * Sends desired packet over rdt connection.
 * @param rdt connection descriptor
 * @param port remote port where to send packet
 * @param packet packet to send
 * @return 0 on error 1 on success
 */
int send_packet(rdt_t *rdt, in_port_t port, packet_t *packet) {
    // data packet requies data :)
    if (packet->cnt_code == C_DAT && (packet->data == NULL || packet->datalen == 0))
        return 0;

    // non data packet requires NULL data and 0 nbytes
    if (packet->cnt_code != C_DAT && (packet->data != NULL || packet->datalen != 0))
        return 0;

    size_t packet_size = packet->cnt_code == C_DAT ? HEADER_LEN + packet->datalen : NON_DATA_PACKET_LEN;
    char *packetbuf = malloc(packet_size);
    if (packetbuf == NULL)
        return 0;

    create_packet(packetbuf, packet_size, packet->number, packet->cnt_code, packet->data, packet->datalen);
    fprintf(stderr, "sending packet to port %d header: %d %d\n", port, packet->number, packet->cnt_code);
    int err = udt_send(rdt->udt, REMOTE_ADDR, port, packetbuf, packet_size);

    free(packetbuf);
    return err;
}

/**
 * Recieves packet from connection from port.
 * @param rdt descriptor
 * @param remote_port only accept packets from this port
 * @retval packet received packet
 * @return 1 on success 0 on error
 */
int recv_packet(rdt_t *rdt, in_port_t remote_port, packet_t *packet) {
    char packbuf[MAX_PACKET_LEN + 1];
    size_t packlen = 0;

    in_port_t port;
    packlen = udt_recv(rdt->udt, packbuf, MAX_PACKET_LEN, NULL, &port);
    if (packlen < HEADER_LEN)           // packet is atleast header length long
        return 0;
    packbuf[packlen] = '\0';

    if (remote_port != 0 && remote_port != port)            // accept only connection from remote_port
        return 0;

    // compute checksum and compare it with checksum in packet header
    uint16_t checksum = compute_checksum(packbuf + 2, packlen - 2);
    uint16_t packsum = *(uint16_t *)packbuf;
    if (packsum != checksum)
        return 0;

    // examine packet number and cnt_code from second header word
    uint16_t pn_cnt = ((uint16_t *)packbuf)[1];
    packet->number = pn_cnt >> 3;
    packet->cnt_code = pn_cnt & 0x7;
    packet->datalen = packlen - HEADER_LEN;
    // if we have some data alloc buffer and copy contents into it
    if (packet->datalen > 0) {
        packet->data = malloc(packet->datalen);
        if (packet->data == NULL)
            return 0;
        memcpy(packet->data, packbuf + HEADER_LEN, packet->datalen);
    } else
        packet->data = NULL;

    fprintf(stderr, "recieved packet from port: %d header: %d %d\n", port, packet->number, packet->cnt_code);

    return 1;
}

/**
 * Wait for single packet to arrive. Or 1sec timeout
 * @param rdt descriptor
 * @param remote_port only accept packets from this port
 * @retval packet received packet
 * @return 1 on success 0 on error
 */
int wait_for_packet(rdt_t *rdt, in_port_t remote_port, packet_t *packet) {
    struct timeval time = {0, TIMEOUT};
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(rdt->udt, &readfds);
    select(rdt->udt+1, &readfds, NULL, NULL, &time);
    if (FD_ISSET(rdt->udt, &readfds)) {
        return recv_packet(rdt, remote_port, packet);
    } else
        return 0;
}

void start_timer(struct itimerval *timer, const sigset_t* sigmask) {
    setitimer(ITIMER_REAL, timer, NULL);
    sigprocmask(SIG_UNBLOCK, sigmask, NULL);
}

void stop_timer(const sigset_t* sigmask) {
    sigprocmask(SIG_BLOCK, sigmask, NULL);
}

void timer_handler(int sig) {
    alarm_arrived = 1;
    fprintf(stderr, "SIGALRM caught\n");
}

int is_pack_in_window(int pack_num, int seq_base, int seq_max) {
    int win_size = seq_max - seq_base;
    if (win_size < 0)
        win_size += SEQUENCE_SIZE;

    int pack_pos = seq_max - pack_num;
    if (pack_pos < 0)
        pack_pos += SEQUENCE_SIZE;

    return pack_pos < win_size;
}

void release_pack_data(pack_data_t pack_data[], int* buf_base) {
    free(pack_data[*buf_base].data);
    pack_data[(*buf_base)++].data = NULL;
    *buf_base = *buf_base % WINDOW_SIZE;
}

void set_pack_data(pack_data_t buff[], int buff_base, int i, char* data, size_t n) {
    i += buff_base;
    i %= WINDOW_SIZE;
    if (buff[i].data == NULL) {
        buff[i].data = data;
        buff[i].n = n;
    }
    else
        free(data);
}

int next_pack_num(int num, int seq_size) {
    return ++num % seq_size;
}

/******************************************************/
/*************** EXPORTED FUNCTIONS *******************/
/******************************************************/

rdt_t* rdt_create(in_port_t local_port) {
    // alloc descriptor
    rdt_t *res = malloc(sizeof(rdt_t));
    if (res == NULL)
        return NULL;

    // try to init udt
    if ((res->udt = udt_init(local_port)) == 0) {
        free(res);
        return NULL;
    }
    res->state = S_CLOSED;

    return res;
}

void rdt_close(rdt_t* rdt) {
    // if established connection send CLOSE packet
    if (rdt->state == S_ESTABLISHED) {
        packet_t pack = {0, C_CLOSE, NULL, 0};
        while (!send_packet(rdt, rdt->remote_port, &pack));
    }
    rdt->state = S_CLOSED;
    free(rdt);
}

int rdt_connect(rdt_t* rdt, in_port_t remote_port) {
    if (rdt->state != S_CLOSED)
        return -1;

    int ret;
    int tries = 0;      // max 5 tries
    do {
        // send establish connection packet
        packet_t pack = { 0, C_EST, NULL, 0};
        if (!send_packet(rdt, remote_port, &pack))
            return -1;

        // recieve packet we are expecting EACK
        ret = wait_for_packet(rdt, remote_port, &pack);
        if (ret) {
            if (pack.data != NULL) {        // EACK doesnt have data
                free(pack.data);
                ret = 0;
            } else if (pack.number == 0 && pack.cnt_code == C_EACK)
                break;
        }
    } while (++tries < 5 && !ret);   // if timeout or bad checksum or wrong packet try again

    if (tries >= 5)
        return -1;

    rdt->state = S_ESTABLISHED;     // successfuly established connection
    rdt->remote_port = remote_port;
    return 0;
}

int rdt_listen(rdt_t* rdt, in_port_t remote_port) {
    if (rdt->state != S_CLOSED)
        return -1;

    rdt->remote_port = remote_port;

    int ret;
    // wait for EST
    do {
        packet_t pack;
        ret = wait_for_packet(rdt, remote_port, &pack);
        if (ret) {
            if (pack.data != NULL) {        // EST doesnt have data
                free(pack.data);
                ret = 0;
            } else if (pack.number == 0 && pack.cnt_code == C_EST) {
                packet_t eack = { 0, C_EACK, NULL, 0 };
                if (!send_packet(rdt, remote_port, &eack))      // send EACK
                    return -1;
                break;
            }
        }
    } while (!ret);

    rdt->state = S_ESTABLISHED;     // successfuly established connection
    return 0;
}

int rdt_send(rdt_t* rdt, FILE* file) {
    if (rdt->state != S_ESTABLISHED)
        return -1;

    // get file descriptor from libc FILE *
    int file_fd = fileno(file);
    fcntl(file_fd, F_SETFL, O_NONBLOCK);   // make reading from file non-clocking
    fprintf(stderr, "FILE*: %p fd: %d\n", (void*)file, file_fd);

    // create queue for packet data
    queue_t queue;
    queue_init(&queue, free);

    signal(SIGALRM, timer_handler);
    struct itimerval timer = {{0, TIMEOUT}, {0, TIMEOUT}};
    sigset_t sigmask;
    sigemptyset(&sigmask);
    sigaddset(&sigmask, SIGALRM);

    // init sequence vars
    int seq_base = 0;
    int seq_max = WINDOW_SIZE - 1;
    int seq_num = 0;

    int ignore_file = 0;
    int file_eof = 0;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(file_fd, &readfds);
    FD_SET(rdt->udt, &readfds);
    int highest_desc = file_fd >= rdt->udt ? file_fd : rdt->udt;
    int ret;
    while ((ret = select(highest_desc + 1, &readfds, NULL, NULL, NULL)) != 0) {
        fprintf(stderr, "select returned %d\n", ret);
        // file ready to read
        if (FD_ISSET(file_fd, &readfds)) {
            fprintf(stderr, "file ready to read\n");
            char *line = NULL;
            size_t linelen = 0;
            ssize_t read = 0;
            // read line from file. if eof end sending packets
            if ((read = getline(&line, &linelen, file)) == -1) {
                fprintf(stderr, "file eof reached\n");
                file_eof = 1;
                if (line)
                    free(line);
            } else {
                // store read line to queue
                queue_data_t data = { line, read };
                if (read > MAX_DATA_LEN + 1 || !queue_append(&queue, &data)) {
                    queue_destroy(&queue);
                    return -1;
                }

                // if we have stored more than window size packet data, ignore file until we ack some packets
                if (queue.size >= WINDOW_SIZE - 1)
                    ignore_file = 1;

                // send packet if available
                int max_pack_num = queue.size >= WINDOW_SIZE - 1 ? seq_max : (seq_base + queue.size) % (SEQUENCE_SIZE);
                fprintf(stderr, "prepare to send packet. seq_num %d seq_base %d q.size %zu\n", seq_num, seq_base, queue.size);
                if (seq_num != max_pack_num) {
                    int i = seq_num - seq_base;
                    if (i < 0)
                        i += SEQUENCE_SIZE;
                    queue_data_t *packdat = queue_get(&queue, i);
                    packet_t pack = { seq_num++, C_DAT, packdat->data, packdat->datalen };
                    if (!send_packet(rdt, rdt->remote_port, &pack))
                        return -1;

                    seq_num %= (SEQUENCE_SIZE);
                }
                start_timer(&timer, &sigmask);  // TODO opravit kdy zacina timer
            }
        }

        // time is out
        if (alarm_arrived) {
            alarm_arrived = 0;

            if (seq_base == seq_num)
                break;

            fprintf(stderr, "resending packets in window\n");

            // resend all available packets in window
            for (int ipack = seq_base; ipack != seq_num;) {
                int i = ipack - seq_base;
                if (i < 0)
                    i += SEQUENCE_SIZE;
                queue_data_t *packdat = queue_get(&queue, i);
                packet_t pack = { ipack++, C_DAT, packdat->data, packdat->datalen };
                if (!send_packet(rdt, rdt->remote_port, &pack))
                    return -1;
                ipack %= (SEQUENCE_SIZE);
            }
            //start_timer(&timer, &sigmask);
        }

        // connection ready to recieve packet
        if (FD_ISSET(rdt->udt, &readfds)) {
            fprintf(stderr, "udt connection ready to read\n");
            // expecting DACK
            packet_t pack;
            if (recv_packet(rdt, rdt->remote_port, &pack)) {
                if (pack.data != NULL)
                    free(pack.data);
                // DACK(n) acknowledges all packets <= n
                // DACK and pack num in window
                else if (pack.cnt_code == C_DACK && is_pack_in_window(pack.number, seq_base, seq_max)) {
                    start_timer(&timer, &sigmask);

                    // packet position relative to window
                    int pack_win_pos = pack.number - seq_base;
                    if (pack_win_pos < 0)
                        pack_win_pos += SEQUENCE_SIZE;

                    seq_max += pack_win_pos;
                    seq_max %= SEQUENCE_SIZE;
                    for (int i = 0; i < pack_win_pos; i++)              // release packet data of acknowledged packets
                        queue_remove(&queue);
                    seq_base = pack.number;
                    ignore_file = 0;

                    // if we have sent all packets and recieved ACK of last one, end sending
                    if (file_eof && pack.number == seq_num)
                        break;
                }
            }
        }

        // reset descriptor set
        FD_ZERO(&readfds);
        if (!ignore_file && !file_eof)
            FD_SET(file_fd, &readfds);
        FD_SET(rdt->udt, &readfds);
    }

    queue_destroy(&queue);

    return 0;
}

int rdt_recv(rdt_t* rdt, FILE* file) {
    fd_set readfds;

    pack_data_t pack_data[WINDOW_SIZE] = { {NULL} };
    int buf_base = 0;

    int seq_base = 0;
    int seq_max = WINDOW_SIZE - 1;

    struct timeval time = {5, 0};

    FD_ZERO(&readfds);
    FD_SET(rdt->udt, &readfds);
    while (select(rdt->udt + 1, &readfds, NULL, NULL, &time)) {
        if (FD_ISSET(rdt->udt, &readfds)) {
            packet_t pack;
            if (recv_packet(rdt, rdt->remote_port, &pack)) {
                // if EACK was lost client could send EST
                if (pack.number == 0 && pack.cnt_code == C_EST) {
                    packet_t eack = { 0, C_EACK, NULL, 0 };
                    if (!send_packet(rdt, rdt->remote_port, &eack))      // send EACK
                        return -1;
                } else if (pack.number == 0 && pack.cnt_code == C_CLOSE) { // on CLOSE packet
                    rdt->state = S_CLOSED;      // close connection
                    break;
                } else if (pack.cnt_code == C_DAT) {
                    fprintf(stderr, "pack num: %d seq_base: %d seq_max: %d\n", pack.number, seq_base, seq_max);
                    if (is_pack_in_window(pack.number, seq_base, seq_max) || pack.number == seq_base) {
                        int pack_win_pos = pack.number - seq_base;
                        if (pack_win_pos < 0)
                            pack_win_pos += SEQUENCE_SIZE;
                        set_pack_data(pack_data, buf_base, pack_win_pos, pack.data, pack.datalen);

                        int j = 0;
                        for (int i = buf_base; j < WINDOW_SIZE; j++, i = next_pack_num(i, WINDOW_SIZE)) {
                            if (pack_data[i].data == NULL)
                                break;

                            fwrite(pack_data[i].data, 1, pack_data[i].n, file);
                            release_pack_data(pack_data, &buf_base);
                        }

                        seq_base += j;
                        seq_base %= SEQUENCE_SIZE;
                        seq_max = seq_base + WINDOW_SIZE;
                        seq_max %= SEQUENCE_SIZE;

                        packet_t dack = { seq_base, C_DACK, NULL, 0};
                        if (!send_packet(rdt, rdt->remote_port, &dack))
                            return -1;
                    } else {
                        packet_t dack = { seq_base, C_DACK, NULL, 0};
                        if (!send_packet(rdt, rdt->remote_port, &dack))
                            return -1;
                        free(pack.data);
                    }
                }
            }
        }

        time.tv_sec = 5;
        FD_ZERO(&readfds);
        FD_SET(rdt->udt, &readfds);
    }

    fflush(file);
    return 0;
}
