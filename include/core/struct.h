#pragma once

#include <stddef.h>

/**
 * @brief container_of 宏
 *
 * 通过一个结构体成员的指针，获取该结构体“容器”的指针。
 *
 * @param ptr 成员的指针
 * @param type 容器结构体的类型 (如 MyStruct)
 * @param member 成员在结构体中的名字 (如 list_node)
 */
#define container_of(ptr, type, member)                                        \
  ((type *)((char *)(ptr) - offsetof(type, member)))
