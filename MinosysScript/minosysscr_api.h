#ifndef MINOSYSSCR_API_H_

// int start(Var **, Engine *, const char *, vector<Var> *);
// 関数の実行
extern "C" int start(void **pret, void *engine, const char *fname, void *args);

#endif // MINOSYSSCR_API_H_

