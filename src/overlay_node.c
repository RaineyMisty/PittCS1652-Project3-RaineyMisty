/*
 * CS 1652 Project 3 
 * (c) Amy Babay, 2022
 * (c) Rainey Chen, 2025
 * 
 * Computer Science Department
 * University of Pittsburgh
 */


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/errno.h>

#include <spu_alarm.h>
#include <spu_events.h>

#include "packets.h"
#include "client_list.h"
#include "node_list.h"
#include "edge_list.h"

#define PRINT_DEBUG 1

#define MAX_CONF_LINE 1024

#define HEARTBEAT_INTERVAL 1
#define HEARTBEAT_TIMEOUT  3

#define MAX_PATH 8
#define MAX_COST 2147483647 //4294967295

#define INITIAL_TTL 5

typedef struct {
    bool received;
    uint32_t seq;
    uint32_t n_links;
    link_t links[MAX_PATH];
} lsa_state_t;

static lsa_state_t lsadb[MAX_PATH+1];

static uint32_t forwarding_table[MAX_PATH+1]; // forwarding_table[i] = next hop to reach i

struct link_state
{
    /* data */
    uint32_t node_id;    // ID of the neighbor node
    uint32_t link_cost;  // cost of the link
    bool     alive;      // true if the link is alive
    int      timeout_id; // timer ID for the heartbeat timeout
};

static struct link_state *link_state_list = NULL;
static int link_state_list_size = 0;

enum mode {
    MODE_NONE,
    MODE_LINK_STATE,
    MODE_DISTANCE_VECTOR,
};

static uint32_t           My_IP      = 0;
static uint32_t           My_ID      = 0;
static uint16_t           My_Port    = 0;

static enum mode          Route_Mode = MODE_NONE;

static struct client_list Client_List;
static struct node_list   Node_List;
static struct edge_list   Edge_List;

static int Client_Sock = -1;
static int Ctrl_Sock   = -1;
static int Data_Sock   = -1;

///////////////////////////
//   Dijkstra process    //
// Writen by Rainey Chen //
///////////////////////////

static void dijkstra_forwarding(void)
{return;
//     int n = Node_List.num_nodes;
//     int dist[MAX_PATH+1]; // dist[i] = cost from My_ID to i
//     bool visited[MAX_PATH+1] = {false};
//     int next_hop[MAX_PATH+1];

//     // Initialize the distance and next hop arrays
//     for (int i = 1; i <= n; i++) {
//         dist[i] = graph[My_ID][i];
//         visited[i] = false;
//         next_hop[i] = (dist[i] == MAX_COST) ? 0 : My_ID;
//     }

//     //  set the non-alive visited to true
//     for (int i = 0; i < link_state_list_size; i++) {
//         if (link_state_list[i].alive == false) {
//             visited[link_state_list[i].node_id] = true;
//             Alarm(DEBUG, "Dijkstra: %u is not alive\n", link_state_list[i].node_id);
//         }
//         else {
//             Alarm(DEBUG, "Dijkstra: %u is alive\n", link_state_list[i].node_id);
//         }
//     }

//     for (int i = 1; i <= n; i++) {
//         if (visited[i] == true) {
//             dist[i] = MAX_COST;
//             next_hop[i] = 0;
//         }
//     }

//     dist[My_ID] = 0;
//     visited[My_ID] = true;
//     next_hop[My_ID] = My_ID;

//     // Dijkstra's algorithm
//     for (int k = 1; k < n; k++) {
//         int min_dist = MAX_COST;
//         int u = -1;

//         // Find the unvisited node with the smallest distance
//         for (int i = 1; i <= n; i++) {
//             if (!visited[i] && dist[i] < min_dist) {
//                 min_dist = dist[i];
//                 u = i;
//             }
//         }

//         // print 
//         printf("dijk: in loop %d, u = %d, min_dist = %d\n", k, u, min_dist);
            
//         if (u == -1) {
//             break; // All remaining nodes are unreachable
//         }
//         visited[u] = true;
//         for (int v = 1; v <= n; v++) {
//             if (!visited[v] && graph[u][v] != MAX_COST) {
//                 int new_dist = dist[u] + graph[u][v];
//                 if (new_dist < dist[v]) {
//                     dist[v] = new_dist;
//                     // next_hop[v] = u;
//                     // Here we determine next_hop[v]:
//                     // - If u is the source (My_ID), it means v is our direct neighbor,
//                     // then the first hop is v itself;
//                     // - Otherwise, u is not the source, and we have recorded in next_hop[u]
//                     // "who is the first hop to u", so the first hop to v
//                     // should follow u's first hop:

//                     // Let me think about this...

//                     // u is the shortest node whose path can be updated
//                     if (u == My_ID) {
//                         next_hop[v] = v;
//                     } else {
//                         next_hop[v] = next_hop[u];
//                     }
//                 }
//             }
//         }
//     }
//     // Update the forwarding table
//     for (int i = 1; i <= n; i++) {
//         forwarding_table[i] = next_hop[i];
//         Alarm(DEBUG, "forwarding_table[%d] = %d in dist = %d\n", i, forwarding_table[i], dist[i]);
//     }
}

static void recompute_route(void)
{
    Alarm(DEBUG, "Recompute Route\n");

    // TODO: Dijkstra

    // TODO: Update the forwarding table
}

///////////////////////////
//   Flooding process    //
// Writen by Rainey Chen //
///////////////////////////

static void flooding(void)
{
    Alarm(DEBUG, "FLOODING\n");

    //// STEP 1: create a packet
    struct lsa_pkt pkt;

    //// STEP 2: set the packet header, origin, ttl, seq
    static uint32_t seq = 0;
    pkt.hdr.type = CTRL_LSA;
    pkt.hdr.src_id = My_ID;
    pkt.hdr.dst_id = 0; // broadcast, set later
    pkt.origin = My_ID;
    pkt.ttl = INITIAL_TTL;
    pkt.seq = ++seq;

    //// STEP 3: get the node link
    //// STEP 4: set the packet link
    int count = 0;
    for (int i = 0; i < link_state_list_size; i++) {
        if (link_state_list[i].alive == true) {
            pkt.links[count].link_id = link_state_list[i].node_id;
            pkt.links[count].link_cost = link_state_list[i].link_cost;
            count++;
        }
    }
    pkt.n_links = count;

    //// STEP 5: update local lsadb
    lsadb[My_ID].received = true;
    lsadb[My_ID].seq = pkt.seq;
    lsadb[My_ID].n_links = pkt.n_links;
    memcpy(lsadb[My_ID].links, pkt.links, sizeof(pkt.links));

    // debug: print the pkt
    printf("[Packet Info]\n");
    printf("pkt.hdr.type = %u\n", pkt.hdr.type);
    printf("pkt.hdr.src_id = %u\n", pkt.hdr.src_id);
    printf("pkt.hdr.dst_id = %u\n", pkt.hdr.dst_id);
    printf("pkt.origin = %u\n", pkt.origin);
    printf("pkt.ttl = %u\n", pkt.ttl);
    printf("pkt.seq = %u\n", pkt.seq);
    printf("pkt.n_links = %u\n", pkt.n_links);
    for (int i = 0; i < pkt.n_links; i++) {
        printf("pkt.links[%d].link_id = %u\n", i, pkt.links[i].link_id);
        printf("pkt.links[%d].link_cost = %u\n", i, pkt.links[i].link_cost);
    }

    // debug: print the lsadb
    printf("[LSA DB]\n");
    for (int i = 1; i <= Node_List.num_nodes; i++) {
        printf("lsadb[%d].received = %u\n", i, lsadb[i].received);
        printf("lsadb[%d].seq = %u\n", i, lsadb[i].seq);
        printf("lsadb[%d].n_links = %u\n", i, lsadb[i].n_links);
        for (int j = 0; j < lsadb[i].n_links; j++) {
            printf("lsadb[%d].links[%d].link_id = %u\n", i, j, lsadb[i].links[j].link_id);
            printf("lsadb[%d].links[%d].link_cost = %u\n", i, j, lsadb[i].links[j].link_cost);
        }
    }
    printf("\n");

    ///// STEP 6: send it to all neighbors
    for (int i = 0; i < link_state_list_size; i++) {
        if (link_state_list[i].alive == true) {
            pkt.hdr.dst_id = link_state_list[i].node_id;
            struct sockaddr_in addr = Node_List.nodes[link_state_list[i].node_id-1]->ctrl_addr; // init with 0!
            sendto(Ctrl_Sock, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                   sizeof(addr));
            Alarm(DEBUG, "Flooding: Sent lsa to %u at first pass\n", link_state_list[i].node_id);
        }
    }
}

static void connect_flooding(void)
{
    Alarm(DEBUG, "CONNECT FLOODING\n");


}

static void delete_flooding(void)
{
    Alarm(DEBUG, "DELETE FLOODING\n");

}

///////////////////////////
//  Forwarding process   //
// Writen by Rainey Chen //
///////////////////////////

/* Forward the packet to the next-hop node based on forwarding table */
void forward_data(struct data_pkt *pkt)
{
    Alarm(DEBUG, "overlay_node: forwarding data to overlay node %u, client port "
                 "%u\n", pkt->hdr.dst_id, pkt->hdr.dst_port);
    /*
     * Students fill in! Do forwarding table lookup, update path information in
     * header (see deliver_locally for an example), and send packet to next hop
     * */
}

/* Deliver packet to one of my local clients */
void deliver_locally(struct data_pkt *pkt)
{
    int path_len = 0;
    int bytes = 0;
    int ret = -1;
    struct client_conn *c = get_client_from_port(&Client_List, pkt->hdr.dst_port);

    /* Check whether we have a local client with this port to deliver to. If
     * not, nothing to do */
    if (c == NULL) {
        Alarm(PRINT, "overlay_node: received data for client that does not "
                     "exist! overlay node %d : client port %u\n",
                     pkt->hdr.dst_id, pkt->hdr.dst_port);
        return;
    }

    Alarm(DEBUG, "overlay_node: Delivering data locally to client with local "
                 "port %d\n", c->data_local_port);

    /* stamp packet so we can see the path taken */
    path_len = pkt->hdr.path_len;
    if (path_len < MAX_PATH) {
        pkt->hdr.path[path_len] = My_ID;
        pkt->hdr.path_len++;
    }

    /* Send data to client */
    bytes = sizeof(struct data_pkt) - MAX_PAYLOAD_SIZE + pkt->hdr.data_len;
    ret = sendto(c->data_sock, pkt, bytes, 0,
                 (struct sockaddr *)&c->data_remote_addr,
                 sizeof(c->data_remote_addr));
    if (ret < 0) {
        Alarm(PRINT, "Error sending to client with sock %d %d:%d\n",
              c->data_sock, c->data_local_port, c->data_remote_port);
        goto err;
    }

    return;

err:
    remove_client_with_sock(&Client_List, c->control_sock);
}

/* Handle incoming data message from another overlay node. Check whether we
 * need to deliver locally to a connected client, or forward to the next hop
 * overlay node */
void handle_overlay_data(int sock, int code, void *data)
{
    int bytes;
    struct data_pkt pkt;
    struct sockaddr_in recv_addr;
    socklen_t fromlen;

    Alarm(DEBUG, "overlay_node: received overlay data msg!\n");

    fromlen = sizeof(recv_addr);
    bytes = recvfrom(sock, &pkt, sizeof(pkt), 0, (struct sockaddr *)&recv_addr,
                     &fromlen);
    if (bytes < 0) {
        Alarm(EXIT, "overlay node: Error receiving overlay data: %s\n",
              strerror(errno));
    }

    /* If there is data to forward, find next hop and forward it */
    if (pkt.hdr.data_len > 0) {
        { /* log packet for debugging */
            char tmp_payload[MAX_PAYLOAD_SIZE+1];
            memcpy(tmp_payload, pkt.payload, pkt.hdr.data_len);
            tmp_payload[pkt.hdr.data_len] = '\0';
            Alarm(DEBUG, "Got forwarded data packet of %d bytes: %s\n",
                  pkt.hdr.data_len, tmp_payload);
        }

        if (pkt.hdr.dst_id == My_ID) {
            deliver_locally(&pkt);
        } else {
            forward_data(&pkt);
        }
    }
}

///////////////////////////
//   Heartbeat process   //
// Writen by Rainey Chen //
///////////////////////////

/* Respond to heartbeat message by sending heartbeat echo */
void handle_heartbeat(struct heartbeat_pkt *pkt)
{
    if (pkt->hdr.type != CTRL_HEARTBEAT) {
        Alarm(PRINT, "Error: non-heartbeat msg in handle_heartbeat\n");
        return;
    }

    // Alarm(DEBUG, "Got heartbeat from %d\n", pkt->hdr.src_id);

     /* Students fill in! */

     // Just create one and send back :)
    struct heartbeat_echo_pkt echo_pkt;
    echo_pkt.hdr.type = CTRL_HEARTBEAT_ECHO;
    echo_pkt.hdr.src_id = My_ID;
    echo_pkt.hdr.dst_id = pkt->hdr.src_id;

    struct sockaddr_in addr = Node_List.nodes[pkt->hdr.src_id-1]->ctrl_addr; // init with 0!
    sendto(Ctrl_Sock, &echo_pkt, sizeof(echo_pkt), 0, (struct sockaddr *)&addr,
           sizeof(addr)); // fine :D
}

/* Broadcast function to tell all others node about the link state
 * Writen by Rainey Chen */
static void broadcast_link_state(void)
{return;
    // // create a lsa packet
    // struct lsa_pkt pkt;
    // pkt.hdr.type = CTRL_LSA;
    // pkt.hdr.src_id = My_ID;

    // // fill in the link state
    // uint32_t count = 0;
    // // for (int i = 0; i < Edge_List.num_edges; i++) {
    // //     if (Edge_List.edges[i]->src_id == My_ID) {
    // //         pkt.link_ids[count] = Edge_List.edges[i]->dst_id;
    // //         pkt.link_costs[count] = Edge_List.edges[i]->cost;
    // //         count++;
    // //     }
    // // }
    // ////////////////////////////////
    // // only send the alive links  //
    // // that is the only way I can //
    // // use to solve this problem  //
    // ////////////////////////////////
    // for (int i = 0; i < link_state_list_size; i++) {
    //     if (link_state_list[i].alive == true) {
    //         pkt.link_ids[count] = link_state_list[i].node_id;
    //         pkt.link_costs[count] = Edge_List.edges[i]->cost;
    //         count++;
    //     }
    // }
    // pkt.num_links = count;

    // // send the packet to all neighbors

    // for (int i = 0; i < link_state_list_size; i++) {
    //     //dist
    //     pkt.hdr.dst_id = link_state_list[i].node_id;
    //     struct sockaddr_in addr = Node_List.nodes[link_state_list[i].node_id-1]->ctrl_addr; // init with 0!
    //     sendto(Ctrl_Sock, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
    //            sizeof(addr));
    //     Alarm(DEBUG, "Sent lsa to %u\n", link_state_list[i].node_id);
    // }
}

/* Callback function for heartbeat timeout.
 * Writen by Rainey Chen */
void heartbeat_timeout_callback(int uc, void *ud)
{
    struct link_state *link = (struct link_state *)ud;
    
    Alarm(DEBUG, "Link %u is dead\n", link->node_id);

    link->alive = false;
    // lsadb[link->node_id].seq++;
    // lsadb[My_ID].links[link->node_id].
    // lsadb[My_ID].n_links--;
    Alarm(DEBUG, "Heartbeat echo: Link %u is false\n", link->node_id);

    // if lsa
    // updating the lsadb for this node
    // if (Route_Mode == MODE_LINK_STATE) {
    //     // lsadb[My_ID].seq++;
    //     int count = 0;
    //     for (int i = 0; i < link_state_list_size; i++) {
    //         if (link_state_list[i].alive == true) {
    //             lsadb[My_ID].links[count].link_id = link_state_list[i].node_id;
    //             lsadb[My_ID].links[count].link_cost = link_state_list[i].link_cost;
    //             count++;
    //         }
    //     }
    //     if (count != lsadb[My_ID].n_links - 1) {
    //         Alarm(PRINT, "Warning: Link state count mismatch, expected %u, got %u\n",
    //               lsadb[My_ID].n_links - 1, count);
    //     }
    //     lsadb[My_ID].n_links = count;
    // }

    // if lsa
    // Delete the dead node
    if (Route_Mode == MODE_LINK_STATE) {
        // lsadb[link->node_id].seq++;
        memset(lsadb[link->node_id].links, 0, sizeof(lsadb[link->node_id].links));
        lsadb[link->node_id].n_links = 0;
        lsadb[link->node_id].seq = 0;
    }

    // Broadcast the link state update
    Alarm(DEBUG, "Link %u is dead -- Flooding to all\n", link->node_id);
    flooding();

    recompute_route();

}

/* Handle heartbeat echo. This indicates that the link is alive, so update our
 * link weights and send update if we previously thought this link was down.
 * Push forward timer for considering the link dead */
void handle_heartbeat_echo(struct heartbeat_echo_pkt *pkt)
{
    if (pkt->hdr.type != CTRL_HEARTBEAT_ECHO) {
        Alarm(PRINT, "Error: non-heartbeat_echo msg in "
                     "handle_heartbeat_echo\n");
        return;
    }

    // Alarm(DEBUG, "Got heartbeat_echo from %d\n", pkt->hdr.src_id);

     /* Students fill in! */

     // get the id from the packet
    uint32_t id = pkt->hdr.src_id;
    // find the link state in the list
    for (int i = 0; i < link_state_list_size; i++) {
        if (link_state_list[i].node_id == id) {
            // check if the link is alive
            if (link_state_list[i].alive == false) {
                // Broadcast the link state update
                link_state_list[i].alive = true;
                Alarm(DEBUG, "Link %u is alive -- Broadcast to all\n", id);
                
                //
                // update the lsadb for this node
                // lsadb[link_state_list[i].node_id].
                flooding();
                // update the lsadb for this node
                recompute_route();
            }
            // reset the timeout timer
            if (link_state_list[i].timeout_id != -1) {
                E_detach_fd(link_state_list[i].timeout_id, 0);
            }
            sp_time timeout = { .sec = HEARTBEAT_TIMEOUT, .usec = 0 };
            link_state_list[i].timeout_id =
                E_queue(heartbeat_timeout_callback, 0, &link_state_list[i], timeout);
            break;
        }
    }
}

///////////////////////////
//  Handle LSA process   //
// Writen by Rainey Chen //
///////////////////////////

/* Process received link state advertisement */
void handle_lsa(struct lsa_pkt *pkt)
{
    if (pkt->hdr.type != CTRL_LSA) {
        Alarm(PRINT, "Error: non-lsa msg in handle_lsa\n");
        return;
    }

    if (Route_Mode != MODE_LINK_STATE) {
        Alarm(PRINT, "Error: LSA msg but not in link state routing mode\n");
    }

    Alarm(DEBUG, "Handle LSA\n");
    Alarm(DEBUG, "Got lsa from %d in origin %u\n", pkt->hdr.src_id, pkt->origin);

     /* Students fill in! */

    //// STEP 1: Send back ack

    //// STEP 2: Check the sequence number
    if (pkt->seq <= lsadb[pkt->origin].seq) {
        Alarm(DEBUG, "LSA: %u is old\n", pkt->origin);
        return;
    }

    //// STEP 3: Update the LSA database (and id alive)

    lsadb[pkt->origin].received = true;
    lsadb[pkt->origin].seq = pkt->seq;
    lsadb[pkt->origin].n_links = pkt->n_links;
    memcpy(lsadb[pkt->origin].links, pkt->links, sizeof(pkt->links));

    // make id alive
    // if (pkt->origin == pkt->hdr.src_id) { // Send from neighber
    //     // printf("LSA: it send from neighbor\n");
    //     for (int i = 0; i < link_state_list_size; i++) {
    //         if (link_state_list[i].node_id == pkt->origin) {
    //             link_state_list[i].alive = true;
    //             Alarm(DEBUG, "LSA: Link %u is alive\n", pkt->origin);
    //             break;
    //         }
    //     }
    // }
    // else {
    //     // printf("LSA: it send from other node\n");
    // }

    // print the lsadb
    printf("[Handle][LSA DB]\n");
    for (int i = 1; i <= Node_List.num_nodes; i++) {
        printf("lsadb[%d].received = %u\n", i, lsadb[i].received);
        printf("lsadb[%d].seq = %u\n", i, lsadb[i].seq);
        printf("lsadb[%d].n_links = %u\n", i, lsadb[i].n_links);
        for (int j = 0; j < lsadb[i].n_links; j++) {
            printf("lsadb[%d].links[%d].link_id = %u\n", i, j, lsadb[i].links[j].link_id);
            printf("lsadb[%d].links[%d].link_cost = %u\n", i, j, lsadb[i].links[j].link_cost);
        }
    }
    printf("\n");

    // update My_ID
    // if neighbor
    // if (pkt->hdr.src_id == pkt->origin) {
    //     lsadb[My_ID].
    // }

    //// STEP 4: flooding this lsa_pkt to all neighbors
    if (--pkt->ttl > 0) {
        for (int i = 0; i < link_state_list_size; i++) {
            if (link_state_list[i].alive == true) {
                if(link_state_list[i].node_id == pkt->hdr.src_id) {
                    continue; // skip the sender
                }
                if(link_state_list[i].node_id == pkt->origin) {
                    continue; // skip the origin
                }
                struct sockaddr_in addr = Node_List.nodes[link_state_list[i].node_id-1]->ctrl_addr; // init with 0!
                sendto(Ctrl_Sock, pkt, sizeof(*pkt), 0, (struct sockaddr *)&addr,
                       sizeof(addr));
                Alarm(DEBUG, "LSA: Sent lsa to %u for flooding %u\n",
                      link_state_list[i].node_id, INITIAL_TTL - pkt->ttl);
            }
        }
    }

    //// STEP 5: Check the map is complete

    //// STEP 6: Recompute the route
    recompute_route();
}

/* Process received distance vector update */
void handle_dv(struct dv_pkt *pkt)
{
    if (pkt->hdr.type != CTRL_DV) {
        Alarm(PRINT, "Error: non-dv msg in handle_dv\n");
        return;
    }

    if (Route_Mode != MODE_DISTANCE_VECTOR) {
        Alarm(PRINT, "Error: Distance Vector Update msg but not in distance "
                     "vector routing mode\n");
    }

    Alarm(DEBUG, "Got dv from %d\n", pkt->hdr.src_id);

     /* Students fill in! */
}

/* Process received overlay control message. Identify message type and call the
 * relevant "handle" function */
void handle_overlay_ctrl(int sock, int code, void *data)
{
    char buf[MAX_CTRL_SIZE];
    struct sockaddr_in recv_addr;
    socklen_t fromlen;
    struct ctrl_hdr * hdr = NULL;
    int bytes = 0;

    // Alarm(DEBUG, "overlay_node: received overlay control msg!\n");

    fromlen = sizeof(recv_addr);
    bytes = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *)&recv_addr,
                     &fromlen);
    if (bytes < 0) {
        Alarm(EXIT, "overlay node: Error receiving ctrl message: %s\n",
              strerror(errno));
    }
    hdr = (struct ctrl_hdr *)buf;

    /* sanity check */
    if (hdr->dst_id != My_ID) {
        Alarm(PRINT, "overlay_node: Error: got ctrl msg with invalid dst_id: "
              "%d\n", hdr->dst_id);
    }

    if (hdr->type == CTRL_HEARTBEAT) {
        /* handle heartbeat */
        handle_heartbeat((struct heartbeat_pkt *)buf);
    } else if (hdr->type == CTRL_HEARTBEAT_ECHO) {
        /* handle heartbeat echo */
        handle_heartbeat_echo((struct heartbeat_echo_pkt *)buf);
    } else if (hdr->type == CTRL_LSA) {
        /* handle link state update */
        handle_lsa((struct lsa_pkt *)buf);
    } else if (hdr->type == CTRL_DV) {
        /* handle distance vector update */
        handle_dv((struct dv_pkt *)buf);
    }
}

void handle_client_data(int sock, int unused, void *data)
{
    int ret, bytes;
    struct data_pkt pkt;
    struct sockaddr_in recv_addr;
    socklen_t fromlen;
    struct client_conn *c;

    Alarm(DEBUG, "Handle client data\n");
    
    c = (struct client_conn *) data;
    if (sock != c->data_sock) {
        Alarm(EXIT, "Bad state! sock %d != data sock\n", sock, c->data_sock);
    }

    fromlen = sizeof(recv_addr);
    bytes = recvfrom(sock, &pkt, sizeof(pkt), 0, (struct sockaddr *)&recv_addr,
                     &fromlen);
    if (bytes < 0) {
        Alarm(PRINT, "overlay node: Error receiving from client: %s\n",
              strerror(errno));
        goto err;
    }

    /* Special case: initial data packet from this client. Use it to set the
     * source port, then ack it */
    if (c->data_remote_port == 0) {
        c->data_remote_addr = recv_addr;
        c->data_remote_port = ntohs(recv_addr.sin_port);
        Alarm(DEBUG, "Got initial data msg from client with sock %d local port "
                     "%u remote port %u\n", sock, c->data_local_port,
                     c->data_remote_port);

        /* echo pkt back to acknowledge */
        ret = sendto(c->data_sock, &pkt, bytes, 0,
                     (struct sockaddr *)&c->data_remote_addr,
                     sizeof(c->data_remote_addr));
        if (ret < 0) {
            Alarm(PRINT, "Error sending to client with sock %d %d:%d\n", sock,
                  c->data_local_port, c->data_remote_port);
            goto err;
        }
    }

    /* If there is data to forward, find next hop and forward it */
    if (pkt.hdr.data_len > 0) {
        { /* log packet for debugging */
            char tmp_payload[MAX_PAYLOAD_SIZE+1];
            memcpy(tmp_payload, pkt.payload, pkt.hdr.data_len);
            tmp_payload[pkt.hdr.data_len] = '\0';
            Alarm(DEBUG, "Got data packet of %d bytes: %s\n", pkt.hdr.data_len, tmp_payload);
        }

        /* Set up header with my info */
        pkt.hdr.src_id = My_ID;
        pkt.hdr.src_port = c->data_local_port;

        /* Deliver / Forward */
        if (pkt.hdr.dst_id == My_ID) {
            deliver_locally(&pkt);
        } else {
            forward_data(&pkt);
        }
    }

    return;

err:
    remove_client_with_sock(&Client_List, c->control_sock);
    
}

void handle_client_ctrl_msg(int sock, int unused, void *data)
{
    int bytes_read = 0;
    int bytes_sent = 0;
    int bytes_expected = sizeof(struct conn_req_pkt);
    struct conn_req_pkt rcv_req;
    struct conn_ack_pkt ack;
    int ret = -1;
    int ret_code = 0;
    char * err_str = "client closed connection";
    struct sockaddr_in saddr;
    struct client_conn *c;

    Alarm(DEBUG, "Client ctrl message, sock %d\n", sock);

    /* Get client info */
    c = (struct client_conn *) data;
    if (sock != c->control_sock) {
        Alarm(EXIT, "Bad state! sock %d != data sock\n", sock, c->control_sock);
    }

    if (c == NULL) {
        Alarm(PRINT, "Failed to find client with sock %d\n", sock);
        ret_code = -1;
        goto end;
    }

    /* Read message from client */
    while (bytes_read < bytes_expected &&
           (ret = recv(sock, ((char *)&rcv_req)+bytes_read,
                       sizeof(rcv_req)-bytes_read, 0)) > 0) {
        bytes_read += ret;
    }
    if (ret <= 0) {
        if (ret < 0) err_str = strerror(errno);
        Alarm(PRINT, "Recv returned %d; Removing client with control sock %d: "
                     "%s\n", ret, sock, err_str);
        ret_code = -1;
        goto end;
    }

    if (c->data_local_port != 0) {
        Alarm(PRINT, "Received req from already connected client with sock "
                     "%d\n", sock);
        ret_code = -1;
        goto end;
    }

    /* Set up UDP socket requested for this client */
    if ((c->data_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        Alarm(PRINT, "overlay_node: client UDP socket error: %s\n", strerror(errno));
        ret_code = -1;
        goto send_resp;
    }

    /* set server address */
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(rcv_req.port);

    /* bind UDP socket */
    if (bind(c->data_sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        Alarm(PRINT, "overlay_node: client UDP bind error: %s\n", strerror(errno));
        ret_code = -1;
        goto send_resp;
    }

    /* Register socket with event handling system */
    ret = E_attach_fd(c->data_sock, READ_FD, handle_client_data, 0, c, MEDIUM_PRIORITY);
    if (ret < 0) {
        Alarm(PRINT, "Failed to register client UDP sock in event handling system\n");
        ret_code = -1;
        goto send_resp;
    }

send_resp:
    /* Send response */
    if (ret_code == 0) { /* all worked correctly */
        c->data_local_port = rcv_req.port;
        ack.id = My_ID;
    } else {
        ack.id = 0;
    }
    bytes_expected = sizeof(ack);
    Alarm(DEBUG, "Sending response to client with control sock %d, UDP port "
                 "%d\n", sock, c->data_local_port);
    while (bytes_sent < bytes_expected) {
        ret = send(sock, ((char *)&ack)+bytes_sent, sizeof(ack)-bytes_sent, 0);
        if (ret < 0) {
            Alarm(PRINT, "Send error for client with sock %d (removing...): "
                         "%s\n", sock, strerror(ret));
            ret_code = -1;
            goto end;
        }
        bytes_sent += ret;
    }

end:
    if (ret_code != 0 && c != NULL) remove_client_with_sock(&Client_List, sock);
}

void handle_client_conn(int sock, int unused, void *data)
{
    int conn_sock;
    struct client_conn new_conn;
    struct client_conn *ret_conn;
    int ret;

    Alarm(DEBUG, "Handle client connection\n");

    /* Accept the connection */
    conn_sock = accept(sock, NULL, NULL);
    if (conn_sock < 0) {
        Alarm(PRINT, "accept error: %s\n", strerror(errno));
        goto err;
    }

    /* Set up the connection struct for this new client */
    new_conn.control_sock     = conn_sock;
    new_conn.data_sock        = -1;
    new_conn.data_local_port  = 0;
    new_conn.data_remote_port = 0;
    ret_conn = add_client_to_list(&Client_List, new_conn);
    if (ret_conn == NULL) {
        goto err;
    }

    /* Register the control socket for this client */
    ret = E_attach_fd(new_conn.control_sock, READ_FD, handle_client_ctrl_msg,
                      0, ret_conn, MEDIUM_PRIORITY);
    if (ret < 0) {
        goto err;
    }

    return;

err:
    if (conn_sock >= 0) close(conn_sock);
}

void init_overlay_data_sock(int port)
{
    int ret = -1;
    struct sockaddr_in saddr;

    if ((Data_Sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        Alarm(EXIT, "overlay_node: data socket error: %s\n", strerror(errno));
    }

    /* set server address */
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(port);

    /* bind listening socket */
    if (bind(Data_Sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        Alarm(EXIT, "overlay_node: data bind error: %s\n", strerror(errno));
    }

    /* Register socket with event handling system */
    ret = E_attach_fd(Data_Sock, READ_FD, handle_overlay_data, 0, NULL, MEDIUM_PRIORITY);
    if (ret < 0) {
        Alarm(EXIT, "Failed to register overlay data sock in event handling system\n");
    }

}

void init_overlay_ctrl_sock(int port)
{
    int ret = -1;
    struct sockaddr_in saddr;

    if ((Ctrl_Sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        Alarm(EXIT, "overlay_node: ctrl socket error: %s\n", strerror(errno));
    }

    /* set server address */
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(port);

    /* bind listening socket */
    if (bind(Ctrl_Sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        Alarm(EXIT, "overlay_node: ctrl bind error: %s\n", strerror(errno));
    }

    /* Register socket with event handling system */
    ret = E_attach_fd(Ctrl_Sock, READ_FD, handle_overlay_ctrl, 0, NULL, MEDIUM_PRIORITY);
    if (ret < 0) {
        Alarm(EXIT, "Failed to register overlay ctrl sock in event handling system\n");
    }
}

void init_client_sock(int client_port)
{
    int ret = -1;
    struct sockaddr_in saddr;

    if ((Client_Sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        Alarm(EXIT, "overlay_node: client socket error: %s\n", strerror(errno));
    }

    /* set server address */
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(client_port);

    /* bind listening socket */
    if (bind(Client_Sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        Alarm(EXIT, "overlay_node: client bind error: %s\n", strerror(errno));
    }

    /* start listening */
    if (listen(Client_Sock, 32) < 0) {
        Alarm(EXIT, "overlay_node: client bind error: %s\n", strerror(errno));
        exit(-1);
    }

    /* Register socket with event handling system */
    ret = E_attach_fd(Client_Sock, READ_FD, handle_client_conn, 0, NULL, MEDIUM_PRIORITY);
    if (ret < 0) {
        Alarm(EXIT, "Failed to register client sock in event handling system\n");
    }

}

void heartbeat_callback(int uc, void *ud)
{
    // Send heartbeat to all neighbors
    for (int i = 0; i < link_state_list_size; i++) {
        struct heartbeat_pkt pkt;
        u_int32_t neighbor_id = link_state_list[i].node_id;
        pkt.hdr.type = CTRL_HEARTBEAT;
        pkt.hdr.src_id = My_ID;
        pkt.hdr.dst_id = neighbor_id;

        struct sockaddr_in addr = Node_List.nodes[neighbor_id-1]->ctrl_addr; // init with 0!
        sendto(Ctrl_Sock, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
               sizeof(addr));
        // Alarm(DEBUG, "Sent heartbeat to %u\n", neighbor_id);

    }

    // Set up next heartbeat
    sp_time heartbeat_interval = { .sec = HEARTBEAT_INTERVAL, .usec = 0 };
    E_queue(heartbeat_callback, 0, NULL, heartbeat_interval);
}

void init_link_state(void)
{
    Alarm(DEBUG, "init link state\n");

    // Check node list is not empty
    if (Node_List.num_nodes == 0) {
        Alarm(EXIT, "overlay_node: error: no nodes in node list\n");
    }
    // Check edge list is not empty
    if (Edge_List.num_edges == 0) {
        Alarm(EXIT, "overlay_node: error: no edges in edge list\n");
    }

    // Allocate space for link state list
    link_state_list_size = 0;
    for (int i = 0; i < Edge_List.num_edges; i++) {
        if (Edge_List.edges[i]->src_id == My_ID) {
            link_state_list_size++;
        }
    }
    link_state_list = calloc(link_state_list_size, sizeof(struct link_state));
    int idx = 0;
    for (int i = 0; i < Edge_List.num_edges; i++) {
        if (Edge_List.edges[i]->src_id == My_ID) {
            link_state_list[idx].node_id = Edge_List.edges[i]->dst_id;
            link_state_list[idx].link_cost = Edge_List.edges[i]->cost;
            link_state_list[idx].alive = false;
            link_state_list[idx].timeout_id = -1;
            idx++;
        }
    }
    
    // init forwarding table
    for (int i = 1; i <= Node_List.num_nodes; i++) {
        forwarding_table[i] = 0;
    }

    // init lsadb
    for (int i = 1; i <= Node_List.num_nodes; i++) {
        lsadb[i].seq = 0;
        lsadb[i].n_links = 0;
        for (int j = 0; j < MAX_PATH; j++) {
            lsadb[i].links[j].link_id = 0;
            lsadb[i].links[j].link_cost = 0;
        }
    }

    // flooding
    // flooding();
    //useless

    // Set up heartbeat timer
    sp_time zero = { .sec = 0, .usec = 0 };
    E_queue(heartbeat_callback, 0, NULL, zero);
}

void init_distance_vector(void)
{
    Alarm(DEBUG, "init distance vector\n");
}

uint32_t ip_from_str(char *ip)
{
    struct in_addr addr;

    inet_pton(AF_INET, ip, &addr);
    return ntohl(addr.s_addr);
}

void process_conf(char *fname, int my_id)
{
    char     buf[MAX_CONF_LINE];
    char     ip_str[MAX_CONF_LINE];
    FILE *   f        = NULL;
    uint32_t id       = 0;
    uint16_t port     = 0;
    uint32_t src      = 0;
    uint32_t dst      = 0;
    uint32_t cost     = 0;
    int node_sec_done = 0;
    int ret           = -1;
    struct node n;
    struct edge e;
    struct node *retn = NULL;
    struct edge *rete = NULL;

    Alarm(DEBUG, "Processing configuration file %s\n", fname);

    /* Open configuration file */
    f = fopen(fname, "r");
    if (f == NULL) {
        Alarm(EXIT, "overlay_node: error: failed to open conf file %s : %s\n",
              fname, strerror(errno));
    }

    /* Read list of nodes from conf file */
    while (fgets(buf, MAX_CONF_LINE, f)) {
        Alarm(DEBUG, "Read line: %s", buf);

        if (!node_sec_done) {
            // sscanf
            ret = sscanf(buf, "%u %s %hu", &id, ip_str, &port);
            Alarm(DEBUG, "    Node ID: %u, Node IP %s, Port: %u\n", id, ip_str, port);
            if (ret != 3) {
                Alarm(DEBUG, "done reading nodes\n");
                node_sec_done = 1;
                continue;
            }

            if (id == my_id) {
                Alarm(DEBUG, "Found my ID (%u). Setting IP and port\n", id);
                My_Port = port;
                My_IP = ip_from_str(ip_str);
            }

            n.id = id;
            n.next_hop = NULL;
            /* set up data address */
            memset(&n.data_addr, 0, sizeof(n.data_addr));
            n.data_addr.sin_family = AF_INET;
            n.data_addr.sin_addr.s_addr = htonl(ip_from_str(ip_str));
            n.data_addr.sin_port = htons(port);
            /* set up control address. note that we use port+1 for the control
             * port. */
            memset(&n.ctrl_addr, 0, sizeof(n.ctrl_addr));
            n.ctrl_addr.sin_family = AF_INET;
            n.ctrl_addr.sin_addr.s_addr = htonl(ip_from_str(ip_str));
            n.ctrl_addr.sin_port = htons(port+1);

            /* add to list of nodes */
            retn = add_node_to_list(&Node_List, n);
            if (retn == NULL) {
                Alarm(EXIT, "Failed to add node to list\n");
            }

        } else { /* Edge section */
            ret = sscanf(buf, "%u %u %u", &src, &dst, &cost);
            Alarm(DEBUG, "    Src ID: %u, Dst ID %u, Cost: %u\n", src, dst, cost);
            if (ret != 3) {
                Alarm(DEBUG, "done reading nodes\n");
                node_sec_done = 1;
                continue;
            }

            e.src_id = src;
            e.dst_id = dst;
            e.cost = cost;
            e.src_node = get_node_from_id(&Node_List, e.src_id);
            e.dst_node = get_node_from_id(&Node_List, e.dst_id);
            if (e.src_node == NULL || e.dst_node == NULL) {
                Alarm(EXIT, "Failed to find node for edge (%u, %u)\n", src, dst);
            }
            rete = add_edge_to_list(&Edge_List, e);
            if (rete == NULL) {
                Alarm(EXIT, "Failed to add edge to list\n");
            }
        }
    }
}

int 
main(int argc, char ** argv) 
{

    char * conf_fname    = NULL;

    if (PRINT_DEBUG) {
        Alarm_set_types(DEBUG);
    }

    /* parse args */
    if (argc != 4) {
        Alarm(EXIT, "usage: overlay_node <id> <config_file> <mode: LS/DV>\n");
    }

    My_ID      = atoi(argv[1]);
    conf_fname = argv[2];

    if (!strncmp("LS", argv[3], 3)) {
        Route_Mode = MODE_LINK_STATE;
    } else if (!strncmp("DV", argv[3], 3)) {
        Route_Mode = MODE_DISTANCE_VECTOR;
    } else {
        Alarm(EXIT, "Invalid mode %s: should be LS or DV\n", argv[5]);
    }

    Alarm(DEBUG, "My ID             : %d\n", My_ID);
    Alarm(DEBUG, "Configuration file: %s\n", conf_fname);
    Alarm(DEBUG, "Mode              : %d\n\n", Route_Mode);

    process_conf(conf_fname, My_ID);
    Alarm(DEBUG, "My IP             : "IPF"\n", IP(My_IP));
    Alarm(DEBUG, "My Port           : %u\n", My_Port);

    { /* print node and edge lists from conf */
        int i;
        struct node *n;
        struct edge *e;
        for (i = 0; i < Node_List.num_nodes; i++) {
            n = Node_List.nodes[i];
            Alarm(DEBUG, "Node %u : data_addr: "IPF":%u\n"
                         "         ctrl_addr: "IPF":%u\n",
                  n->id,
                  IP(ntohl(n->data_addr.sin_addr.s_addr)),
                  ntohs(n->data_addr.sin_port),
                  IP(ntohl(n->ctrl_addr.sin_addr.s_addr)),
                  ntohs(n->ctrl_addr.sin_port));
        }

        for (i = 0; i < Edge_List.num_edges; i++) {
            e = Edge_List.edges[i];
            Alarm(DEBUG, "Edge (%u, %u) : "IPF":%u -> "IPF":%u\n",
                  e->src_id, e->dst_id,
                  IP(ntohl(e->src_node->data_addr.sin_addr.s_addr)),
                  ntohs(e->src_node->data_addr.sin_port),
                  IP(ntohl(e->dst_node->data_addr.sin_addr.s_addr)),
                  ntohs(e->dst_node->data_addr.sin_port));
        }
    }
    
    /* Initialize event system */
    E_init();

    /* Set up TCP socket for client connection requests */
    init_client_sock(My_Port);

    /* Set up UDP sockets for sending and receiving messages from other
     * overlay nodes */
    init_overlay_data_sock(My_Port);
    init_overlay_ctrl_sock(My_Port+1);

    if (Route_Mode == MODE_LINK_STATE) {
        init_link_state();
    } else {
        init_distance_vector();
    }

    /* Enter event handling loop */
    Alarm(DEBUG, "Entering event loop!\n");
    E_handle_events();

    return 0;
}
