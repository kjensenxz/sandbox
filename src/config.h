#ifndef _SANDBOX_CONFIG_H_
#define _SANDBOX_CONFIG_H_

vmconf_t *vmconfig(char *);
char **initargv(vmconf_t *conf);
char **startargv(vmconf_t *conf);

#endif
