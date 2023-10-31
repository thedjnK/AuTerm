#ifndef SMP_GROUP_ARRAY_H
#define SMP_GROUP_ARRAY_H

#include <smp_group_fs_mgmt.h>
#include <smp_group_img_mgmt.h>
#include <smp_group_os_mgmt.h>
#include <smp_group_settings_mgmt.h>
#include <smp_group_shell_mgmt.h>
#include <smp_group_stat_mgmt.h>
#include <smp_group_zephyr_mgmt.h>

struct smp_group_array {
    smp_group_fs_mgmt *fs_mgmt;
    smp_group_img_mgmt *img_mgmt;
    smp_group_os_mgmt *os_mgmt;
    smp_group_settings_mgmt *settings_mgmt;
    smp_group_shell_mgmt *shell_mgmt;
    smp_group_stat_mgmt *stat_mgmt;
    smp_group_zephyr_mgmt *zephyr_mgmt;
};

#endif // SMP_GROUP_ARRAY_H
