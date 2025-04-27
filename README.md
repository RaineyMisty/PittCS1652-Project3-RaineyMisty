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
  - If one neighbor died, flooding!

- `LSA` packet

### Dijkstra process

> Aim: Only need to process dijkstra

### Flooding process

> Aim: Use function flooding() and packet sent

### Forwarding process

### Heartbeat process

- For heartbeat message, only need to reply
- For heartbeat echo message
  - The node are repeatedly sending heartbeat message to all neighbors
  - If receiving heartbeat echo message, that means the node is alive
    - This case means, if this node is dead before, then it means it start.
    - But this case we don't need to flooding, because a new started node will send lsa packet to flooding
  - If one neighbor died, then we need to flooding
    - All the neighbors of the dead node will start to flooding
    1. This node will delete the edge to the dead node
    2. This node will will flooding to ask others to delete the edge
    3. This node will delete the neighbor node from the map
    4. This node will receive lsa packets from others to delete the edge that the dead node privously linked

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

- `lsa_state_t` to store the state for all node in the network
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

#### Init LSA Step

- Step1: Init the link_state_list, storing all the edges of neighbors
- Step2: Init the lsadb
  - set all the data to 0, because there is no link
  - the avilable data are received only when receiving the lsa packet
  - so, no flooding, no recompute
- Step3: Set a timer to send heartbeat packet