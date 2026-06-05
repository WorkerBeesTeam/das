#!/bin/bash

SCRIPT_PATH=$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )
LOCK_PATH="$SCRIPT_PATH/$(basename "$0").lock"
if [ -f "$LOCK_PATH" ]; then
	T1=$(cat "$LOCK_PATH")
	NOW=$(date +%s)
	DELTA=$((NOW-T1))
	if [ $DELTA -lt 600 ]; then
		>&2 echo "Abort becouse lock file present"
		exit 1
	fi
fi

date +%s > "$LOCK_PATH"

dbus-send --system --print-reply --dest=ru.deviceaccess.Das.Server / ru.deviceaccess.Das.iface.fill_log_value_layers

rm "$LOCK_PATH"
