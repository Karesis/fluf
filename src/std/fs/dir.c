#include <core/msg/asrt.h>
#include <std/fs/dir.h>
#include <std/fs/path.h>
#include <std/string/cstr.h> // 依赖 allocer_strdup
#include <std/string/string.h>

// --- POSIX 依赖 ---
// (这个模块是 POSIX 特定的，这是 `fluf` 的一个明确选择)
#include <dirent.h>   // opendir, readdir, closedir
#include <sys/stat.h> // stat
#include <unistd.h>   // (stat)

/**
 * @brief dir_walk 的公共 API 实现
 */
bool dir_walk(allocer_t *alc, const char *base_path, dir_walk_callback_fn *cb,
              void *userdata) {
  asrt_msg(alc && base_path && cb, "dir_walk arguments cannot be NULL");

  DIR *dir = opendir(base_path);
  if (dir == NULL) {
    // (不是目录, 或无权访问)
    return false;
  }

  // 我们使用 `alc` 分配一个临时的字符串构建器
  string_t path_builder;
  if (!string_init(&path_builder, alc, 256)) { // 256 字节初始路径
    closedir(dir);
    return false; // OOM
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    const char *name = entry->d_name;

    // 1. 跳过 "." 和 ".."
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
      continue;
    }

    // 2. 构建完整路径
    string_clear(&path_builder);
    path_join(&path_builder, slice_from_cstr(base_path), slice_from_cstr(name));

    // (我们必须立即获取 c_str，因为 builder 马上会被重用)
    const char *full_path_cstr = string_as_cstr(&path_builder);

    // 3. Stat (获取文件类型)
    struct stat statbuf;
    if (stat(full_path_cstr, &statbuf) != 0) {
      // (可能是个坏的 symlink, 跳过)
      continue;
    }

    dir_entry_type_t type;
    if (S_ISDIR(statbuf.st_mode)) {
      type = DIR_ENTRY_DIR;
    } else if (S_ISREG(statbuf.st_mode)) {
      type = DIR_ENTRY_FILE;
    } else {
      type = DIR_ENTRY_OTHER;
    }

    // 4. (关键!) 为回调分配一个*稳定*的副本
    //    这解决了 `string_as_cstr` 悬垂指针的陷阱
    char *arena_path = cstr_dup(full_path_cstr, alc);
    if (!arena_path) {
      // (OOM)
      closedir(dir);
      string_destroy(&path_builder);
      return false;
    }

    // 5. 调用回调
    cb(arena_path, type, userdata);

    // 6. 递归
    if (type == DIR_ENTRY_DIR) {
      // (我们传递 `arena_path`，它是稳定的)
      dir_walk(alc, arena_path, cb, userdata);
    }
  }

  closedir(dir);
  string_destroy(&path_builder);
  return true;
}