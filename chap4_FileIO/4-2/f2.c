// p.88
// 4-2_freopen()の使い方
// freopen(): 既にオープンしてあるストリームを別の出力先に結びつける関数 (元のストリームは閉じられる)
// デバッグに有用

#include <stdio.h>

int main(int argc, char *argv[]) {
  FILE *fp;

  fp = freopen("./debug.log", "w", stdout);

  printf("Content-type:text/html\n\n");
  printf("<html>\n");
  printf("<head>\n");
  printf("<META HTTP-EQUIV=\"Content-type\"CONTENT=\"text/html; charset=x-euc\">\n");
  printf("<title>テスト</title>");
  printf("</head>\n");
  printf("<body>\n");
  printf("<p>テストです</p>\n");
  printf("</body>\n");
  printf("</html>\n");

  // 本来はここに fclose() が必要

  return 0;
}
