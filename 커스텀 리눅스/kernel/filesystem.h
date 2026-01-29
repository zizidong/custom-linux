#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdint.h>
#include <stddef.h>

// 파일 타입
typedef enum {
    FS_TYPE_FILE,
    FS_TYPE_DIRECTORY,
    FS_TYPE_SYMLINK,
    FS_TYPE_DEVICE
} fs_type_t;

// 파일 권한
typedef enum {
    FS_PERM_READ = 0x01,
    FS_PERM_WRITE = 0x02,
    FS_PERM_EXECUTE = 0x04,
    FS_PERM_OWNER_READ = 0x0100,
    FS_PERM_OWNER_WRITE = 0x0200,
    FS_PERM_OWNER_EXECUTE = 0x0400,
    FS_PERM_GROUP_READ = 0x0010,
    FS_PERM_GROUP_WRITE = 0x0020,
    FS_PERM_GROUP_EXECUTE = 0x0040,
    FS_PERM_OTHER_READ = 0x0001,
    FS_PERM_OTHER_WRITE = 0x0002,
    FS_PERM_OTHER_EXECUTE = 0x0004
} fs_permissions_t;

// 파일 열기 모드
typedef enum {
    FS_OPEN_READ = 0x01,
    FS_OPEN_WRITE = 0x02,
    FS_OPEN_APPEND = 0x04,
    FS_OPEN_CREATE = 0x08,
    FS_OPEN_TRUNCATE = 0x10
} fs_open_mode_t;

// 파일 정보 구조체
typedef struct {
    uint32_t inode;           // inode 번호
    fs_type_t type;          // 파일 타입
    uint32_t size;           // 파일 크기
    uint32_t permissions;    // 권한
    uint32_t owner;          // 소유자 ID
    uint32_t group;          // 그룹 ID
    uint32_t created_time;   // 생성 시간
    uint32_t modified_time;  // 수정 시간
    uint32_t accessed_time;  // 접근 시간
} fs_stat_t;

// 파일 핸들 구조체
typedef struct {
    uint32_t fd;             // 파일 디스크립터
    uint32_t inode;          // inode 번호
    uint32_t offset;         // 현재 오프셋
    fs_open_mode_t mode;     // 열기 모드
    uint32_t ref_count;      // 참조 카운트
} fs_file_t;

// 디렉토리 엔트리 구조체
typedef struct {
    uint32_t inode;          // inode 번호
    char name[256];          // 파일명
    fs_type_t type;         // 파일 타입
} fs_dirent_t;

// 파일 시스템 함수들
int fs_init(void);
int fs_mount(const char* device, const char* mount_point);
int fs_unmount(const char* mount_point);

// 파일 조작 함수들
int fs_open(const char* path, fs_open_mode_t mode);
int fs_close(int fd);
ssize_t fs_read(int fd, void* buffer, size_t size);
ssize_t fs_write(int fd, const void* buffer, size_t size);
int fs_seek(int fd, int offset, int whence);
int fs_stat(const char* path, fs_stat_t* stat);
int fs_fstat(int fd, fs_stat_t* stat);

// 디렉토리 조작 함수들
int fs_mkdir(const char* path, uint32_t permissions);
int fs_rmdir(const char* path);
int fs_opendir(const char* path);
int fs_readdir(int dir_fd, fs_dirent_t* entry);
int fs_closedir(int dir_fd);

// 파일 시스템 관리 함수들
int fs_create(const char* path, fs_type_t type, uint32_t permissions);
int fs_delete(const char* path);
int fs_rename(const char* old_path, const char* new_path);
int fs_link(const char* target, const char* link_path);
int fs_symlink(const char* target, const char* link_path);
int fs_chmod(const char* path, uint32_t permissions);
int fs_chown(const char* path, uint32_t owner, uint32_t group);

// 파일 시스템 정보
uint32_t fs_get_free_space(const char* path);
uint32_t fs_get_total_space(const char* path);
int fs_sync(void);

// 경로 관련 함수들
char* fs_getcwd(char* buffer, size_t size);
int fs_chdir(const char* path);
int fs_absolute_path(const char* relative, char* absolute, size_t size);
int fs_normalize_path(const char* path, char* normalized, size_t size);

// 파일 시스템 타입
typedef struct filesystem {
    const char* name;
    int (*mount)(const char* device, const char* mount_point);
    int (*unmount)(const char* mount_point);
    int (*open)(const char* path, fs_open_mode_t mode);
    int (*close)(int fd);
    ssize_t (*read)(int fd, void* buffer, size_t size);
    ssize_t (*write)(int fd, const void* buffer, size_t size);
    int (*seek)(int fd, int offset, int whence);
    int (*stat)(const char* path, fs_stat_t* stat);
    int (*mkdir)(const char* path, uint32_t permissions);
    int (*rmdir)(const char* path);
    int (*delete)(const char* path);
    int (*rename)(const char* old_path, const char* new_path);
} filesystem_t;

// 파일 시스템 등록
int fs_register(const char* name, filesystem_t* fs);
int fs_unregister(const char* name);

// 마운트 포인트 관리
typedef struct mount_point {
    char device[256];
    char mount_point[256];
    filesystem_t* fs;
    void* private_data;
    struct mount_point* next;
} mount_point_t;

// 마운트 포인트 함수들
mount_point_t* fs_find_mount_point(const char* path);
int fs_add_mount_point(const char* device, const char* mount_point, filesystem_t* fs);
int fs_remove_mount_point(const char* mount_point);

#endif // FILESYSTEM_H
