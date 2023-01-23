FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "${@bb.utils.contains('DISTRO_FEATURES', 'dac', 'file://container.cfg', '', d)}"

CMDLINE_append = "${@bb.utils.contains('DISTRO_FEATURES', 'dac', 'cgroup_enable=cpuset cgroup_enable=memory cgroup_memory=1', '', d)}"
