#ifndef PANIC_H
#define PANIC_H

/**
 * @brief Log an issue and halt the system
 * 
 * @param location The location where the issue was encountered
 * @param msg A message explaining the issue
*/
[[noreturn]] void panic(char *location, char *msg);

#endif