// p.84
// 4-1_ディレクトリ内容の読み出し
// ls コマンドを呼び出さずにディレクトリの内容を得るプログラム

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[]) {
  DIR *dir;
  struct dirent *dp, Entry, *Result;
  char path[512], buf[512];
  int ret;

  // 引数指定
  if (argc <= 1) strcpy(path, ".");
  else strcpy(path, argv[1]);

  // エラー処理
  if ((dir=opendir(path)) == NULL) {
    perror("opendir");
    return -1;
  }

  // readdir() は NULL がリターンするまでループ (NULL: 末尾 or エラー(要errno調べ))
  printf("[readdir version]\n");
  for (dp = readdir(dir); dp != NULL; dp = readdir(dir)) {
    printf("%s\n", dp -> d_name);
  }
  if (errno != 0) perror("readdir");
  rewinddir(dir);

  // readdir_r() は 末尾: 第3引数にNULL, エラー: 0以外がリターン
  printf("[readdir_r version]\n");
  while(1) {
    ret = readdir_r(dir, &Entry, &Result);
    if (ret != 0) {
      perror("readdir_r");
      break;
    }
    if (Result == NULL) break;
    printf("%s\n", Result -> d_name);
  }

  closedir(dir);
  return 0;
}
