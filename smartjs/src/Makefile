sys_config.c,sys_config.h: fs/conf_sys_defaults.json
	@python ../../tools/json_to_c_config.py $< sys_config
	@$(MAKE) -C .. format

pre-commit: sys_config.h
