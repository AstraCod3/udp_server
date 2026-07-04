#!/bin/bash

# ==============================================================================
# CONFIGURAZIONE
# ==============================================================================
# Cambia l'IP e la Porta in base a dove è in ascolto il tuo udp_server C++
SERVER_IP="127.0.0.1"
SERVER_PORT=12345
LOOP_COUNT=10
DELAY=0.5 # 500 millisecondi

# ==============================================================================
# GENERAZIONE DEL PAYLOAD SEQUENZIALE
# ==============================================================================
echo "Generazione della sequenza di byte in corso..."

# OPZIONE 1: 256 byte esatti (da 00 a FF) -> [00, 01, 02, ..., FE, FF]
HEX_SEQUENCE=$(printf '%02x' {0..255})

# OPZIONE 2: 255 byte (da 01 a FF) -> Se preferisci saltare lo zero iniziale,
# scommenta la riga sotto (togli il #) e commenta quella sopra.
# HEX_SEQUENCE=$(printf '%02x' {1..255})

# ==============================================================================
# CICLO DI TRASMISSIONE UDP
# ==============================================================================
echo "Inizio invio di $LOOP_COUNT pacchetti a $SERVER_IP:$SERVER_PORT..."
echo "Pausa tra i pacchetti: ${DELAY}s"
echo "----------------------------------------------------------------"

for ((i=1; i<=LOOP_COUNT; i++))
do
    # 1. Prende la stringa esadecimale (es. "000102...ffff")
    # 2. xxd -r -p la converte nei byte binari grezzi corrispondenti
    # 3. nc -u spedisce il pacchetto via UDP al server
    echo -n "$HEX_SEQUENCE" | xxd -r -p | nc -u "$SERVER_IP" "$SERVER_PORT"
    
    echo "[Pacchetto $i/$LOOP_COUNT] Sequenza inviata con successo."
    
    # Attende 500ms prima del prossimo invio
    sleep "$DELAY"
done

echo "----------------------------------------------------------------"
echo "Invio completato. Tutti i $LOOP_COUNT pacchetti sono stati trasmessi."

