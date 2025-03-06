#pragma once

[[noreturn]] void halt(void);

__attribute((format(printf, 1, 2)))
[[noreturn]] void panic(const char *fmt, ...);
