#ifndef IPC_MSG_H
#define IPC_MSG_H

#define STRING_MSG              "string_msg"
//#define STRING_MSG_FMT                "[char: 256]"
#define STRING_MSG_FMT          "string"

#define HOGE01_MSG              "hoge01_msg"

#define HOGE02_MSG              "hoge02_msg"

#define HOGE01_MSG_FMT          "{char, [double: 10]}"

#define HOGE02_MSG_FMT          "{int, [double: 20]}"


/* wlSync */
#define SOC_MSG			"soc"

#define SOC_MSG_FMT		"int"

#define DIRLIST_MSG    		"dir_list"

#define DIRLIST_MSG_FMT		"string"

#define OK_MSG			"ok"

#define OK_MSG_FMT		"string"

#define NOOP_MSG		"noop"

#define NOOP_MSG_FMT		"string"

#endif /* IPC_MSG_H */

