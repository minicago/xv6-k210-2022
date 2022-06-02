#include "kernel/include/types.h"
#include "kernel/include/stat.h"
#include "xv6-user/user.h"

/*
 * 测试成功时输出：
 * "  Hello, mmap success"
 * 测试失败时输出：
 * "mmap error."
 */
static struct stat kst;
void test_mmap(void){
    char *array;
    const char *str = "  Hello, mmap successfully!";
    int fd;

    fd = open("test_mmap.txt", O_RDWR | O_CREATE);
    write(fd, str, strlen(str));
    fstat(fd, &kst);
    printf("file len: %d\n", kst.size);
    array = mmap(NULL, kst.size, PROT_WRITE | PROT_READ, MAP_FILE | MAP_SHARED, fd, 0);
    array[5] = 'a';
    //printf("return array: %x\n", array);

    if (array == MAP_FAILED) {
	printf("mmap error.\n");
    }else{
	printf("mmap content: %s\n", array);
    printf("mmap positon: %p\n", array);
	printf("%s\n", str);
    //sleep(100000000);
    munmap(array, kst.size);
    //printf("End!");
// sleep(10000000);
    }

    close(fd);
    
}

int main(void){
    test_mmap();
    return 0;
}
