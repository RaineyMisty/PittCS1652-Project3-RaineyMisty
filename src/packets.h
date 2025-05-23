#define MAX_PAYLOAD_SIZE 1300
#define MAX_CTRL_SIZE 1300
#define MAX_PATH 8

struct data_pkt_hdr {
    uint32_t dst_id; /* ID of destination overlay node */
    uint32_t src_id; /* ID of source overlay node; only filled in by overlay node (not client) */
    uint16_t dst_port; /* port where dst client is connected to dst overlay node */
    uint16_t src_port; /* only filled in by overlay node */
    uint32_t data_len;
    uint32_t path_len; /* length of path */
    uint8_t  path[8]; /* sequence of overlay nodes this packet passed through */
} __attribute__((packed));

struct data_pkt {
    struct data_pkt_hdr hdr;
    char                payload[MAX_PAYLOAD_SIZE];
} __attribute__((packed));

/* The type field in the struct ctrl_hdr should be one of these values to
 * identify the type of the packet */
#define CTRL_HEARTBEAT      1
#define CTRL_HEARTBEAT_ECHO 2
#define CTRL_LSA            3
#define CTRL_DV             4

struct ctrl_hdr {
    uint32_t type;
    uint32_t src_id;
    uint32_t dst_id;
} __attribute__((packed));

struct heartbeat_pkt {
    struct ctrl_hdr hdr;
    /* you may add additional fields here (if needed) */
} __attribute__((packed));

struct heartbeat_echo_pkt {
    struct ctrl_hdr hdr;
    /* you may add additional fields here (if needed) */
} __attribute__((packed));

enum trigger_type {
    NODE_NONE = -1,     // -1 NONE
    NODE_CONNECT = 0,   // 0 CONNECT
    NODE_PORT_1 = 1,    // 1 deleted from port 1
    NODE_PORT_2 = 2,
    NODE_PORT_3 = 3,
    NODE_PORT_4 = 4,
    NODE_PORT_5 = 5,
    NODE_PORT_6 = 6,
    NODE_PORT_7 = 7,
    NODE_PORT_8 = 8,
};

typedef struct {
    uint32_t link_id;
    uint32_t link_cost;
} link_t;

/* Link state advertisement */
struct lsa_pkt {
    struct ctrl_hdr hdr;
    /* you should define the fields needed here */
    uint32_t origin;
    enum trigger_type trigger;
    uint32_t ttl;
    uint32_t seq;
    uint32_t n_links; /* number of links in the packet */
    link_t links[MAX_PATH]; /* array of links */
} __attribute__((packed));

/* Distance vector update */
struct dv_pkt {
    struct ctrl_hdr hdr;
    /* you should define the fields needed here */
} __attribute__((packed));


/* These packets are only used for establishing a client connection to an
 * overlay node. You should not need to modify these */
struct conn_req_pkt {
    uint16_t port;
} __attribute__((packed));

struct conn_ack_pkt {
    uint32_t id;
} __attribute__((packed));

