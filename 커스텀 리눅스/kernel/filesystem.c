#include "filesystem.h"
#include "memory.h"
#include <string.h>

#define MAX_FILES 1024
#define MAX_MOUNT_POINTS 16
#define MAX_FILE_SYSTEMS 8

// 전역 변수들
static fs_file_t file_table[MAX_FILES];
static mount_point_t* mount_points = NULL;
static filesystem_t* registered_fs[MAX_FILE_SYSTEMS];
static int next_fd = 3; // 0, 1, 2는 표준 입출력용
static char current_working_directory[256] = "/";

// 파일 시스템 초기화
int fs_init(void) {
    // 파일 테이블 초기화
    memset(file_table, 0, sizeof(file_table));
    
    // 표준 입출력 파일 디스크립터 설정
    file_table[0].fd = 0; // stdin
    file_table[0].mode = FS_OPEN_READ;
    file_table[0].ref_count = 1;
    
    file_table[1].fd = 1; // stdout
    file_table[1].mode = FS_OPEN_WRITE;
    file_table[1].ref_count = 1;
    
    file_table[2].fd = 2; // stderr
    file_table[2].mode = FS_OPEN_WRITE;
    file_table[2].ref_count = 1;
    
    // 마운트 포인트 초기화
    mount_points = NULL;
    
    // 등록된 파일 시스템 초기화
    memset(registered_fs, 0, sizeof(registered_fs));
    
    return 0;
}

// 파일 시스템 등록
int fs_register(const char* name, filesystem_t* fs) {
    for (int i = 0; i < MAX_FILE_SYSTEMS; i++) {
        if (registered_fs[i] == NULL) {
            registered_fs[i] = fs;
            return 0;
        }
    }
    return -1; // 등록 실패
}

// 파일 시스템 등록 해제
int fs_unregister(const char* name) {
    for (int i = 0; i < MAX_FILE_SYSTEMS; i++) {
        if (registered_fs[i] && strcmp(registered_fs[i]->name, name) == 0) {
            registered_fs[i] = NULL;
            return 0;
        }
    }
    return -1;
}

// 마운트 포인트 추가
int fs_add_mount_point(const char* device, const char* mount_point, filesystem_t* fs) {
    mount_point_t* new_mount = (mount_point_t*)kmalloc(sizeof(mount_point_t));
    if (!new_mount) return -1;
    
    strncpy(new_mount->device, device, 255);
    new_mount->device[255] = '\0';
    strncpy(new_mount->mount_point, mount_point, 255);
    new_mount->mount_point[255] = '\0';
    new_mount->fs = fs;
    new_mount->private_data = NULL;
    new_mount->next = mount_points;
    mount_points = new_mount;
    
    return 0;
}

// 마운트 포인트 제거
int fs_remove_mount_point(const char* mount_point) {
    mount_point_t* current = mount_points;
    mount_point_t* prev = NULL;
    
    while (current) {
        if (strcmp(current->mount_point, mount_point) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                mount_points = current->next;
            }
            kfree(current);
            return 0;
        }
        prev = current;
        current = current->next;
    }
    
    return -1;
}

// 마운트 포인트 찾기
mount_point_t* fs_find_mount_point(const char* path) {
    mount_point_t* current = mount_points;
    
    while (current) {
        if (strncmp(path, current->mount_point, strlen(current->mount_point)) == 0) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

// 파일 시스템 마운트
int fs_mount(const char* device, const char* mount_point) {
    // 간단한 구현: 기본 파일 시스템으로 마운트
    filesystem_t* default_fs = NULL;
    
    // 등록된 파일 시스템 중 첫 번째 사용
    for (int i = 0; i < MAX_FILE_SYSTEMS; i++) {
        if (registered_fs[i]) {
            default_fs = registered_fs[i];
            break;
        }
    }
    
    if (!default_fs) return -1;
    
    return fs_add_mount_point(device, mount_point, default_fs);
}

// 파일 시스템 언마운트
int fs_unmount(const char* mount_point) {
    return fs_remove_mount_point(mount_point);
}

// 파일 열기
int fs_open(const char* path, fs_open_mode_t mode) {
    // 빈 파일 디스크립터 찾기
    int fd = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].ref_count == 0) {
            fd = i;
            break;
        }
    }
    
    if (fd == -1) return -1;
    
    // 파일 테이블 엔트리 초기화
    file_table[fd].fd = next_fd++;
    file_table[fd].inode = 0; // 간단한 구현
    file_table[fd].offset = 0;
    file_table[fd].mode = mode;
    file_table[fd].ref_count = 1;
    
    return file_table[fd].fd;
}

// 파일 닫기
int fs_close(int fd) {
    // 파일 디스크립터 찾기
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].fd == fd) {
            file_table[i].ref_count--;
            if (file_table[i].ref_count == 0) {
                memset(&file_table[i], 0, sizeof(fs_file_t));
            }
            return 0;
        }
    }
    
    return -1;
}

// 파일 읽기
ssize_t fs_read(int fd, void* buffer, size_t size) {
    // 간단한 구현: 실제로는 파일 시스템별로 구현
    (void)fd;
    (void)buffer;
    (void)size;
    
    return 0; // 읽기 실패
}

// 파일 쓰기
ssize_t fs_write(int fd, const void* buffer, size_t size) {
    // 간단한 구현: 실제로는 파일 시스템별로 구현
    (void)fd;
    (void)buffer;
    (void)size;
    
    return 0; // 쓰기 실패
}

// 파일 탐색
int fs_seek(int fd, int offset, int whence) {
    // 파일 디스크립터 찾기
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].fd == fd) {
            switch (whence) {
                case 0: // SEEK_SET
                    file_table[i].offset = offset;
                    break;
                case 1: // SEEK_CUR
                    file_table[i].offset += offset;
                    break;
                case 2: // SEEK_END
                    // 파일 크기를 알아야 함
                    break;
            }
            return file_table[i].offset;
        }
    }
    
    return -1;
}

// 파일 정보 가져오기
int fs_stat(const char* path, fs_stat_t* stat) {
    // 간단한 구현
    if (!stat) return -1;
    
    memset(stat, 0, sizeof(fs_stat_t));
    stat->inode = 1;
    stat->type = FS_TYPE_FILE;
    stat->size = 0;
    stat->permissions = FS_PERM_READ | FS_PERM_WRITE;
    stat->owner = 0;
    stat->group = 0;
    
    return 0;
}

// 파일 핸들 정보 가져오기
int fs_fstat(int fd, fs_stat_t* stat) {
    // 간단한 구현
    if (!stat) return -1;
    
    memset(stat, 0, sizeof(fs_stat_t));
    stat->inode = 1;
    stat->type = FS_TYPE_FILE;
    stat->size = 0;
    stat->permissions = FS_PERM_READ | FS_PERM_WRITE;
    
    return 0;
}

// 디렉토리 생성
int fs_mkdir(const char* path, uint32_t permissions) {
    // 간단한 구현
    (void)path;
    (void)permissions;
    
    return 0;
}

// 디렉토리 제거
int fs_rmdir(const char* path) {
    // 간단한 구현
    (void)path;
    
    return 0;
}

// 디렉토리 열기
int fs_opendir(const char* path) {
    // 간단한 구현: 파일 열기와 동일
    return fs_open(path, FS_OPEN_READ);
}

// 디렉토리 읽기
int fs_readdir(int dir_fd, fs_dirent_t* entry) {
    // 간단한 구현
    (void)dir_fd;
    (void)entry;
    
    return -1; // 더 이상 엔트리 없음
}

// 디렉토리 닫기
int fs_closedir(int dir_fd) {
    return fs_close(dir_fd);
}

// 파일 생성
int fs_create(const char* path, fs_type_t type, uint32_t permissions) {
    // 간단한 구현
    (void)path;
    (void)type;
    (void)permissions;
    
    return 0;
}

// 파일 삭제
int fs_delete(const char* path) {
    // 간단한 구현
    (void)path;
    
    return 0;
}

// 파일 이름 변경
int fs_rename(const char* old_path, const char* new_path) {
    // 간단한 구현
    (void)old_path;
    (void)new_path;
    
    return 0;
}

// 하드 링크 생성
int fs_link(const char* target, const char* link_path) {
    // 간단한 구현
    (void)target;
    (void)link_path;
    
    return 0;
}

// 심볼릭 링크 생성
int fs_symlink(const char* target, const char* link_path) {
    // 간단한 구현
    (void)target;
    (void)link_path;
    
    return 0;
}

// 파일 권한 변경
int fs_chmod(const char* path, uint32_t permissions) {
    // 간단한 구현
    (void)path;
    (void)permissions;
    
    return 0;
}

// 파일 소유자 변경
int fs_chown(const char* path, uint32_t owner, uint32_t group) {
    // 간단한 구현
    (void)path;
    (void)owner;
    (void)group;
    
    return 0;
}

// 사용 가능한 공간 가져오기
uint32_t fs_get_free_space(const char* path) {
    // 간단한 구현
    (void)path;
    
    return 1024 * 1024; // 1MB
}

// 전체 공간 가져오기
uint32_t fs_get_total_space(const char* path) {
    // 간단한 구현
    (void)path;
    
    return 10 * 1024 * 1024; // 10MB
}

// 파일 시스템 동기화
int fs_sync(void) {
    // 간단한 구현
    
    return 0;
}

// 현재 작업 디렉토리 가져오기
char* fs_getcwd(char* buffer, size_t size) {
    if (!buffer || size == 0) return NULL;
    
    strncpy(buffer, current_working_directory, size - 1);
    buffer[size - 1] = '\0';
    
    return buffer;
}

// 작업 디렉토리 변경
int fs_chdir(const char* path) {
    if (!path) return -1;
    
    strncpy(current_working_directory, path, 255);
    current_working_directory[255] = '\0';
    
    return 0;
}

// 절대 경로 변환
int fs_absolute_path(const char* relative, char* absolute, size_t size) {
    if (!relative || !absolute || size == 0) return -1;
    
    if (relative[0] == '/') {
        // 이미 절대 경로
        strncpy(absolute, relative, size - 1);
    } else {
        // 상대 경로를 절대 경로로 변환
        strncpy(absolute, current_working_directory, size - 1);
        strncat(absolute, "/", size - strlen(absolute) - 1);
        strncat(absolute, relative, size - strlen(absolute) - 1);
    }
    
    absolute[size - 1] = '\0';
    return 0;
}

// 경로 정규화
int fs_normalize_path(const char* path, char* normalized, size_t size) {
    if (!path || !normalized || size == 0) return -1;
    
    // 간단한 구현: 실제로는 "..", "." 처리 필요
    strncpy(normalized, path, size - 1);
    normalized[size - 1] = '\0';
    
    return 0;
}
