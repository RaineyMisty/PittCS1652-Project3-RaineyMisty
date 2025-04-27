Not complete the project
I'm trying to write a better code
This version is to avoid no submit
I'll upload a new version tomorrow

## Logic when receiving packet

- `Heartbeat` packet
  - Response with `Heartbeat Echo` packet

- `Heartbeat Echo` packet
  - Mark the node who sent the packet as alive
  - Creating a new timer and callback if failed

- `LSA` packet

### Dijkstra process

> Aim: Only need to process dijkstra

### Flooding process

> Aim: Use function flooding() and packet sent

### Forwarding process

### Heartbeat process

### LSA process

> Aim: Start with a new lsa packet, end up with giving a forwarding table

#### struct

- `lsa_packet`
  - `hdr` hdr
  - `origin` origin id
  - `ttl` -1 in every pass
  - `seq` sequence number
  - `n_links` nunber of links
  - `links` to store the link information
    - `link_id` id of the link
    - `link_cost` cost of the link

- `lsa_state` to store the state for all node in the network
  - `received` to store whether the lsa packet are received from that node
  - `seq` to store the lastest sequence number
  - `n_links` number of links
  - `links` to store the link of one node, the whole lsa_state array can be a link map for the network


#### Data

- Use `lsa_database` to store the 

#### Handle LSA Step

- Step1: Send back ack
- Step2: Check the sequence number
- Step3: Update the LSA database
- Step4: Flooding this lsa_pkt to all the neighbors
- Step5: Check the map is complete
- Step6: Update the forwarding table