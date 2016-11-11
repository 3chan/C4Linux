// p.91
// 4-3_プロセス間通信のFIFO (受信プログラム)
// ソケットほど大げさでなく、パイプより自由

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/fcntl.h>

int main() {
  int fd, len;
  char buf[256];

  // FIFO をオープン、ディスクリプタを得る
  if ((fd = open("./FifoTest", O_RDONLY)) == -1) {
    perror("open");
    return -1;
  }

  // 読み込み
  while(1) {
    len = read(fd, buf, sizeof(buf) -1);
    if (len == 0) {
      break;
    }
    buf[len] = '\0';
    fputs(buf, stdout);
  }

  // 終了
  close(fd);
  unlink("./FifoTest"); // FIFO 削除
  return 0;
}
