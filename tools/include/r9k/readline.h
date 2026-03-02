/* rsh - remote shell */
/*
 * readline.h - 终端行输入接口
 *
 * 提供 UTF-8 行输入接口，异常 UTF-8 字节按单字节处理。
 * 不依赖第三方库，无复杂状态机。
 * 在 raw 模式下精确维护光标与缓冲区索引对应关系。
 *
 * author   : Luo Tiansheng
 * created  : 2025/10/24
 */
#ifndef READLINE_H_
#define READLINE_H_

#include <stdlib.h>

char *readline(const char *prompt);
void add_history(const char *line);
const char **get_history(size_t *p_size);
void clear(void);

#endif /*  READLINE_H_ */
