// p.91
// 4-3_プロセス間通信のFIFO (送信プログラム)
// ソケットほど大げさでなく、パイプより自由
// Ctrl + D で終了

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/fcntl.h>

int main() {
  int fd;
  char buf[256];

  // FIFO 作成
  if (mkfifo("./FifoTest", 0666) == 1) {
    perror("mkfifo");
    /* エラー時も継続 */
  }
  // FIFO をオープン、ディスクリプタを得る
  if ((fd = open("./FifoTest", O_WRONLY)) == -1) {
    perror("open");
    return -1;
  }

  // 書き込み
  while(1) {
    fgets(buf, sizeof(buf) - 1, stdin);
    if (feof (stdin)) {
      break;
    }
    write(fd, buf, strlen(buf));
  }

  // 終了
  close(fd);
  return 0;
}
