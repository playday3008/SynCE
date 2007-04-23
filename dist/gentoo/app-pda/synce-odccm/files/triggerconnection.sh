# Trigger connection with PDA
# Module for Gentoo Baselayout 1.12.9
# Send comments to Marcel Selhorst <synce@selhorst.net>

triggerconnection_depend() {
        after dhcp
        provide triggerconnection
}

triggerconnection_start() {
        local iface="$1"
        einfo "Triggerconnection module, checking interface ${iface}"
        INTERFACE_IP="$(/sbin/ifconfig ${iface} | grep inet | sed 's/.*:\(.*.*\) .*/\1/' | cut -d'.' -f1-3)"
        INTERFACE_IP="${INTERFACE_IP}.1"
        einfo "Triggering connection on IP $INTERFACE_IP"
        /usr/local/bin/triggerconnection $INTERFACE_IP
        return $?
}

triggerconnection_stop() {
        return 0
}

triggerconnection_check_installed() {
        [[ -x /usr/local/bin/triggerconnection ]] && return 0
        ${1:-false} && eerror "Please install 'triggerconnection' first! See SynCE-Wiki!"
        return 1
}

# vim: set ts=4 :
