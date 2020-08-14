#!/system/bin/sh
mkfifo -m 0660 /dev/socket/micshm
chown media:media /dev/socket/micshm
chmod 0660 /dev/socket/micshm
