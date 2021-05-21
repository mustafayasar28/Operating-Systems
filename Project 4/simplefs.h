

// Do not change this file //

#define MODE_READ 0
#define MODE_APPEND 1
#define BLOCKSIZE 4096 // bytes

int create_format_vdisk (char *vdiskname, unsigned int  m);

int sfs_mount (char *vdiskname);

int sfs_umount ();

int sfs_create(char *filename);

int sfs_open(char *filename, int mode);

int sfs_close(int fd);

int sfs_getsize (int fd);

int sfs_read(int fd, void *buf, int n);

int sfs_append(int fd, void *buf, int n);

int sfs_delete(char *filename);



