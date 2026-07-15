#!/bin/bash

# ==============================================================================
# CONFIGURATION
# ==============================================================================
# Change the IP and Port based on where your C++ udp_server is listening
#SERVER_IP="127.0.0.1"     
SERVER_IP="192.168.1.5"    
SERVER_PORT=1581           
LOOP_COUNT=1               
DELAY=0.5 # 500 milliseconds

# ==============================================================================
# DECREASING PAYLOAD GENERATION
# ==============================================================================
echo "Generating decreasing byte sequence..."

# Generates 256 bytes in decreasing order (from 255 down to 0) -> [FF, FE, FD, ..., 01, 00]
HEX_SEQUENCE=""
for ((i=255; i>=0; i--))
do
    HEX_SEQUENCE+=$(printf '%02x' "$i")
done

# ==============================================================================
# UDP TRANSMISSION LOOP
# ==============================================================================
echo "Starting transmission of $LOOP_COUNT packets to $SERVER_IP:$SERVER_PORT..."
echo "Delay between packets: ${DELAY}s"
echo "----------------------------------------------------------------"

for ((i=0; i<LOOP_COUNT; i++))
do
    # 1. Takes the hexadecimal string (e.g., "fffefd...0100")
    # 2. xxd -r -p converts it into the corresponding raw binary bytes
    # 3. nc -u sends the UDP packet to the target server
    # echo -n "$HEX_SEQUENCE" | xxd -r -p | nc -u -w 1 "$SERVER_IP" "$SERVER_PORT"
    echo -n "$HEX_SEQUENCE" | xxd -r -p > /dev/udp/"$SERVER_IP"/"$SERVER_PORT"

    COUNT=$((i+1))
    echo "[Packet $COUNT/$LOOP_COUNT] Sequence sent successfully."

    # Waits 500ms before the next transmission
    sleep "$DELAY"
done

echo "----------------------------------------------------------------"
echo "Transmission completed. All $LOOP_COUNT packets have been sent."

