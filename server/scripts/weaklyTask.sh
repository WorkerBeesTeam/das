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

dbConf=`awk '/^\[Database\]/,/^\[/ && !/^\[Database\]/' "${SCRIPT_PATH}/../DasServer.conf"`

export dbName=$(echo "$dbConf" | grep -oP 'Name=\K.*')
export dbHost=$(echo "$dbConf" | grep -oP 'Host=\K.*')
export dbPort=$(echo "$dbConf" | grep -oP 'Port=\K.*')
export dbUser=$(echo "$dbConf" | grep -oP 'User=\K.*')
export dbPass=$(echo "$dbConf" | grep -oP 'Password=\K.*')

cat <<EOF | mysql -h $dbHost -P $dbPort -u $dbUser -p"$dbPass" -D $dbName
DELETE FROM das_log_value
WHERE timestamp_msecs < UNIX_TIMESTAMP(DATE_SUB(NOW(), INTERVAL 1 YEAR)) * 1000;
DELETE FROM das_log_value_minute
WHERE timestamp_msecs < UNIX_TIMESTAMP(DATE_SUB(NOW(), INTERVAL 1 YEAR)) * 1000;
DELETE FROM das_log_value_hour
WHERE timestamp_msecs < UNIX_TIMESTAMP(DATE_SUB(NOW(), INTERVAL 1 YEAR)) * 1000;
DELETE FROM das_log_value_day
WHERE timestamp_msecs < UNIX_TIMESTAMP(DATE_SUB(NOW(), INTERVAL 1 YEAR)) * 1000;
EOF

dbus-send --system --print-reply --dest=ru.deviceaccess.Das.Server / ru.deviceaccess.Das.iface.organize_log_partition

rm "$LOCK_PATH"
