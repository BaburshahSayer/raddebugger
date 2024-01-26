// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef OS_CORE_H
#define OS_CORE_H

////////////////////////////////
//~ rjf: Access Flags

typedef U32 OS_AccessFlags;
enum
{
  OS_AccessFlag_Read       = (1<<0),
  OS_AccessFlag_Write      = (1<<1),
  OS_AccessFlag_Execute    = (1<<2),
  OS_AccessFlag_ShareRead  = (1<<3),
  OS_AccessFlag_ShareWrite = (1<<4),
};

////////////////////////////////
//~ allen: Files

typedef U32 OS_FileIterFlags;
enum
{
  OS_FileIterFlag_SkipFolders     = (1 << 0),
  OS_FileIterFlag_SkipFiles       = (1 << 1),
  OS_FileIterFlag_SkipHiddenFiles = (1 << 2),
  OS_FileIterFlag_Done            = (1 << 31),
};

typedef struct OS_FileIter OS_FileIter;
struct OS_FileIter
{
  OS_FileIterFlags flags;
  U8 memory[600];
};

typedef struct OS_FileInfo OS_FileInfo;
struct OS_FileInfo
{
  String8 name;
  FileProperties props;
};

// nick: on-disk file identifier
typedef struct OS_FileID OS_FileID;
struct OS_FileID
{
  U64 v[3];
};

////////////////////////////////
//~ rjf: System Paths

typedef enum OS_SystemPath
{
  OS_SystemPath_Binary,
  OS_SystemPath_Initial,
  OS_SystemPath_Current,
  OS_SystemPath_UserProgramData,
  OS_SystemPath_ModuleLoad,
}
OS_SystemPath;

typedef enum OS_PathFromUserKind
{
  OS_PathFromUserKind_Save,
  OS_PathFromUserKind_Load,
}
OS_PathFromUserKind;

typedef struct OS_PathFromUser OS_PathFromUser;
struct OS_PathFromUser
{
  OS_PathFromUserKind kind;
  String8 path;
  U64 filter_count;
  String8 *filter_extensions;
  String8 *filter_names;
};

////////////////////////////////
//~ allen: Launch Input

typedef struct OS_LaunchOptions OS_LaunchOptions;
struct OS_LaunchOptions
{
  String8List cmd_line;
  String8 path;
  String8List env;
  B32 inherit_env;
  B32 consoleless;
};

////////////////////////////////
//~ rjf: Handle Type

typedef struct OS_Handle OS_Handle;
struct OS_Handle
{
  U64 u64[1];
};

typedef struct OS_HandleNode OS_HandleNode;
struct OS_HandleNode
{
  OS_HandleNode *next;
  OS_Handle v;
};

typedef struct OS_HandleList OS_HandleList;
struct OS_HandleList
{
  OS_HandleNode *first;
  OS_HandleNode *last;
  U64 count;
};

typedef struct OS_HandleArray OS_HandleArray;
struct OS_HandleArray
{
  OS_Handle *v;
  U64 count;
};

////////////////////////////////
// Time

#define OS_UNIX_TIME_MAX max_U32
typedef U32 OS_UnixTime;

////////////////////////////////
// Global Unique ID

typedef struct OS_Guid
{
  U32 data1;
  U16 data2;
  U16 data3;
  U8  data4[8];
} OS_Guid;
StaticAssert(sizeof(OS_Guid) == 16, os_guid_check);

////////////////////////////////
//~ rjf: Thread Types

typedef void OS_ThreadFunctionType(void *ptr);

////////////////////////////////
//~ rjf: Handle Type Functions (Helpers, Implemented Once)

internal OS_Handle os_handle_zero(void);
internal B32 os_handle_match(OS_Handle a, OS_Handle b);
internal void os_handle_list_push(Arena *arena, OS_HandleList *handles, OS_Handle handle);
internal OS_HandleArray os_handle_array_from_list(Arena *arena, OS_HandleList *list);

////////////////////////////////
//~ rjf: System Path Helper (Helper, Implemented Once)

internal String8 os_string_from_system_path(Arena *arena, OS_SystemPath path);

////////////////////////////////
//~ rjf: Command Line Argc/Argv Helper (Helper, Implemented Once)

internal String8List os_string_list_from_argcv(Arena *arena, int argc, char **argv);

////////////////////////////////
//~ rjf: Process Helpers (Helper, Implemented Once)

internal void os_relaunch_self(void);

////////////////////////////////
//~ rjf: Filesystem Helpers (Helpers, Implemented Once)

internal String8        os_data_from_file_path(Arena *arena, String8 path);
internal B32            os_write_data_to_file_path(String8 path, String8 data);
internal B32            os_write_data_list_to_file_path(String8 path, String8List list);
internal OS_FileID      os_id_from_file_path(String8 path);
internal S64            os_file_id_compare(OS_FileID a, OS_FileID b);
internal String8        os_string_from_file_range(Arena *arena, OS_Handle file, Rng1U64 range);

////////////////////////////////
//~ rjf: Synchronization Primitive Helpers (Helpers, Implemented Once)

internal void os_mutex_take(OS_Handle mutex);
internal void os_mutex_drop(OS_Handle mutex);
internal void os_rw_mutex_take_r(OS_Handle rw_mutex);
internal void os_rw_mutex_drop_r(OS_Handle rw_mutex);
internal void os_rw_mutex_take_w(OS_Handle rw_mutex);
internal void os_rw_mutex_drop_w(OS_Handle rw_mutex);
// returns false on timeout, true on signal, (max_wait_ms = max_U64) -> no timeout
internal B32  os_condition_variable_wait(OS_Handle cv, OS_Handle mutex, U64 endt_us);
internal B32  os_condition_variable_wait_rw_r(OS_Handle cv, OS_Handle rw_mutex, U64 endt_us);
internal B32  os_condition_variable_wait_rw_w(OS_Handle cv, OS_Handle rw_mutex, U64 endt_us);
internal void os_condition_variable_signal(OS_Handle cv);
internal void os_condition_variable_broadcast(OS_Handle cv);

#define OS_MutexScope(mutex) DeferLoop(os_mutex_take(mutex), os_mutex_drop(mutex))
#define OS_MutexScopeR(mutex) DeferLoop(os_rw_mutex_take_r(mutex), os_rw_mutex_drop_r(mutex))
#define OS_MutexScopeW(mutex) DeferLoop(os_rw_mutex_take_w(mutex), os_rw_mutex_drop_w(mutex))
#define OS_MutexScopeRWPromote(mutex) DeferLoop((os_rw_mutex_drop_r(mutex), os_rw_mutex_take_w(mutex)), (os_rw_mutex_drop_w(mutex), os_rw_mutex_take_r(mutex)))

////////////////////////////////
//~ rjf: @os_hooks Main Initialization API (Implemented Per-OS)

internal void os_init(int argc, char **argv);

////////////////////////////////
//~ rjf: @os_hooks Memory Allocation (Implemented Per-OS)

internal void* os_reserve(U64 size);
internal B32   os_commit(void *ptr, U64 size);
internal void* os_reserve_large(U64 size);
internal B32   os_commit_large(void *ptr, U64 size);
internal void  os_decommit(void *ptr, U64 size);
internal void  os_release(void *ptr, U64 size);

internal B32 os_set_large_pages(B32 flag);
internal B32 os_large_pages_enabled(void);
internal U64 os_large_page_size(void);

internal void* os_alloc_ring_buffer(U64 size, U64 *actual_size_out);
internal void  os_free_ring_buffer(void *ring_buffer, U64 actual_size);

////////////////////////////////
//~ rjf: @os_hooks System Info (Implemented Per-OS)

internal String8      os_machine_name(void);
internal U64          os_page_size(void);
internal U64          os_allocation_granularity(void);
internal U64          os_logical_core_count(void);

////////////////////////////////
//~ rjf: @os_hooks Process Info (Implemented Per-OS)

internal String8List os_get_command_line_arguments(void);
internal S32         os_get_pid(void);
internal S32         os_get_tid(void);
internal String8List os_get_environment(void);
internal U64         os_string_list_from_system_path(Arena *arena, OS_SystemPath path, String8List *out);

////////////////////////////////
//~ rjf: @os_hooks Process Control (Implemented Per-OS)

internal void os_exit_process(S32 exit_code);

////////////////////////////////
//~ rjf: @os_hooks File System (Implemented Per-OS)

//- rjf: files
internal OS_Handle      os_file_open(OS_AccessFlags flags, String8 path);
internal void           os_file_close(OS_Handle file);
internal U64            os_file_read(OS_Handle file, Rng1U64 rng, void *out_data);
internal void           os_file_write(OS_Handle file, Rng1U64 rng, void *data);
internal B32            os_file_set_times(OS_Handle file, DateTime time);
internal FileProperties os_properties_from_file(OS_Handle file);
internal OS_FileID      os_id_from_file(OS_Handle file);
internal B32            os_delete_file_at_path(String8 path);
internal B32            os_copy_file_path(String8 dst, String8 src);
internal String8        os_full_path_from_path(Arena *arena, String8 path);
internal B32            os_file_path_exists(String8 path);
internal FileProperties os_properties_from_file_path(String8 path);

//- rjf: file maps
internal OS_Handle os_file_map_open(OS_AccessFlags flags, OS_Handle file);
internal void      os_file_map_close(OS_Handle map);
internal void *    os_file_map_view_open(OS_Handle map, OS_AccessFlags flags, Rng1U64 range);
internal void      os_file_map_view_close(OS_Handle map, void *ptr);

//- rjf: directory iteration
internal OS_FileIter *os_file_iter_begin(Arena *arena, String8 path, OS_FileIterFlags flags);
internal B32          os_file_iter_next(Arena *arena, OS_FileIter *iter, OS_FileInfo *info_out);
internal void         os_file_iter_end(OS_FileIter *iter);

//- rjf: directory creation
internal B32 os_make_directory(String8 path);

////////////////////////////////
//~ rjf: @os_hooks Shared Memory (Implemented Per-OS)

internal OS_Handle os_shared_memory_alloc(U64 size, String8 name);
internal OS_Handle os_shared_memory_open(String8 name);
internal void      os_shared_memory_close(OS_Handle handle);
internal void *    os_shared_memory_view_open(OS_Handle handle, Rng1U64 range);
internal void      os_shared_memory_view_close(OS_Handle handle, void *ptr);

////////////////////////////////
//~ rjf: @os_hooks Time (Implemented Per-OS)

internal OS_UnixTime os_now_unix(void);
internal DateTime    os_now_universal_time(void);
internal DateTime    os_universal_time_from_local_time(DateTime *local_time);
internal DateTime    os_local_time_from_universal_time(DateTime *universal_time);
internal U64         os_now_microseconds(void);
internal void        os_sleep_milliseconds(U32 msec);

////////////////////////////////
//~ rjf: @os_hooks Child Processes (Implemented Per-OS)

internal B32   os_launch_process(OS_LaunchOptions *options, OS_Handle *handle_out);
internal B32   os_process_wait(OS_Handle handle, U64 endt_us);
internal void  os_process_release_handle(OS_Handle handle);

////////////////////////////////
//~ rjf: @os_hooks Threads (Implemented Per-OS)

internal OS_Handle os_launch_thread(OS_ThreadFunctionType *func, void *ptr, void *params);
internal void      os_release_thread_handle(OS_Handle thread);

////////////////////////////////
//~ rjf: @os_hooks Synchronization Primitives (Implemented Per-OS)

// NOTE(allen): Mutexes are recursive - support counted acquire/release nesting
// on a single thread

//- rjf: recursive mutexes
internal OS_Handle os_mutex_alloc(void);
internal void      os_mutex_release(OS_Handle mutex);
internal void      os_mutex_take_(OS_Handle mutex);
internal void      os_mutex_drop_(OS_Handle mutex);

//- rjf: reader/writer mutexes
internal OS_Handle os_rw_mutex_alloc(void);
internal void      os_rw_mutex_release(OS_Handle rw_mutex);
internal void      os_rw_mutex_take_r_(OS_Handle mutex);
internal void      os_rw_mutex_drop_r_(OS_Handle mutex);
internal void      os_rw_mutex_take_w_(OS_Handle mutex);
internal void      os_rw_mutex_drop_w_(OS_Handle mutex);

//- rjf: condition variables
internal OS_Handle os_condition_variable_alloc(void);
internal void      os_condition_variable_release(OS_Handle cv);
// returns false on timeout, true on signal, (max_wait_ms = max_U64) -> no timeout
internal B32       os_condition_variable_wait_(OS_Handle cv, OS_Handle mutex, U64 endt_us);
internal B32       os_condition_variable_wait_rw_r_(OS_Handle cv, OS_Handle mutex_rw, U64 endt_us);
internal B32       os_condition_variable_wait_rw_w_(OS_Handle cv, OS_Handle mutex_rw, U64 endt_us);
internal void      os_condition_variable_signal_(OS_Handle cv);
internal void      os_condition_variable_broadcast_(OS_Handle cv);

//- rjf: cross-process semaphores
internal OS_Handle os_semaphore_alloc(U32 initial_count, U32 max_count, String8 name);
internal void      os_semaphore_release(OS_Handle semaphore);
internal OS_Handle os_semaphore_open(String8 name);
internal void      os_semaphore_close(OS_Handle semaphore);
internal B32       os_semaphore_take(OS_Handle semaphore, U64 endt_us);
internal void      os_semaphore_drop(OS_Handle semaphore);

////////////////////////////////
//~ rjf: @os_hooks Dynamically-Loaded Libraries (Implemented Per-OS)

internal OS_Handle os_library_open(String8 path);
internal VoidProc *os_library_load_proc(OS_Handle lib, String8 name);
internal void      os_library_close(OS_Handle lib);

////////////////////////////////
//~ rjf: @os_hooks Safe Calls (Implemented Per-OS)

internal void os_safe_call(OS_ThreadFunctionType *func, OS_ThreadFunctionType *fail_handler, void *ptr);

////////////////////////////////
//~ rjf: @os_hooks GUIDs (Implemented Per-OS)

internal OS_Guid os_make_guid(void);
internal String8 os_string_from_guid(Arena *arena, OS_Guid guid);

#endif // OS_CORE_H
