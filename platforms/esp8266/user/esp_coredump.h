#ifndef ESP_COREDUMP_INCLUDED
#define ESP_COREDUMP_INCLUDED

/* dump core to fd. if fd is -1 dump to default output */
void esp_dump_core(int fd, struct regfile *);

#endif /* ESP_COREDUMP_INCLUDED */
