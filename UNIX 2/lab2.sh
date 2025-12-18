#!/bin/sh

SHARED_DIR="/data"
COUNTER=1
MAX_INDEX=999
INSTANCE_ID=$(mktemp -u XXX)
echo "Container instance $INSTANCE_ID started"

SYNC_FILE="$SHARED_DIR/.sync_lock"

while true; do
    FILE_TO_USE=""

    {   flock -x 52
        INDEX=1
        while [ $INDEX -le $MAX_INDEX ]; do
            FILENAME=$(printf "%03d" $INDEX)
            FULL_PATH="$SHARED_DIR/$FILENAME"
            
            if [ ! -e "$FULL_PATH" ]; then
                FILE_TO_USE="$FULL_PATH"
                printf "ID: %s | NUM: %d\n" "$INSTANCE_ID" "$COUNTER" > "$FILE_TO_USE"
                break
            fi
            INDEX=$((INDEX+1))
        done
    } 33>"$SYNC_FILE"

    echo "[Created] $FILE_TO_USE (ID: $INSTANCE_ID, NUM: $COUNTER)"

    sleep 1

    rm "$FILE_TO_USE"
    echo "[Deleted] $FILE_TO_USE"

    COUNTER=$((COUNTER+1))
done


