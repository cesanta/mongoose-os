#ifndef CS_FW_PLATFORMS_CC3200_SRC_CC3200_UPDATER_H_
#define CS_FW_PLATFORMS_CC3200_SRC_CC3200_UPDATER_H_

int apply_update(int boot_cfg_idx, struct boot_cfg *cfg);
void commit_update(int boot_cfg_idx, struct boot_cfg *cfg);
void revert_update(int boot_cfg_idx, struct boot_cfg *cfg);

#endif /* CS_FW_PLATFORMS_CC3200_SRC_CC3200_UPDATER_H_ */
