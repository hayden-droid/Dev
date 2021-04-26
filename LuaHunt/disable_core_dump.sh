echo 'kernel.core_uses_pid = 0' >> /etc/sysctl.conf
echo 'kernel.core_pattern = core' >> /etc/sysctl.conf
echo 'fs.suid_dumpable = 0' >> /etc/sysctl.conf

sysctl -p
