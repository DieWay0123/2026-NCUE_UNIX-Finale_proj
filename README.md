# UNIX 系統程式設計期末專題

## 專案簡介

本專案為 UNIX 系統程式設計期末專題，目標是實作一個具備實用性、可維護性，並能展示 UNIX 系統程式設計概念的應用程式。

專案開發過程中會盡量使用課程中學到的技術，例如：

- Process / Thread
- Socket / IPC
- File I/O
- Signal Handling
- Makefile
- Linux System Call
- Benchmark / Performance Measurement
- Modular Programming

本 Repo 主要用於團隊協作、版本控制、程式碼整合與文件管理。

---

## 專案目標

本專案希望達成以下目標：

1. 完成主要功能的可執行程式
2. 使用清楚的專案架構管理程式碼
3. 使用 GitHub 進行團隊協作與版本控管
4. 撰寫清楚的文件，方便成員理解與維護
5. 設計基本測試與 Benchmark，驗證程式正確性與效能表現

---

## 團隊分工

| 成員 | 主要負責內容 | 說明 |
|---|---|---|
| 成員 A | 核心功能開發 | 負責主要程式邏輯、資料流程與功能整合 |
| 成員 B | 系統架構與模組設計 | 負責專案架構規劃、模組拆分與介面設計 |
| 成員 C | Benchmark 與測試 | 負責效能測試、壓力測試、正確性測試與測試資料設計 |
| 成員 D | 文件與 Demo | 負責 README、報告、簡報、使用說明與 Demo 流程整理 |

> 實際開發時可以依照進度互相支援，但每個人應優先完成自己負責的主要項目。

---

## 預計功能

### 基本功能

- 完成主要程式功能
- 支援基本輸入與輸出
- 具備錯誤處理能力
- 可以在 Linux / UNIX-like 環境下編譯與執行

### 延伸功能

- 加入更完整的使用者互動流程
- 支援多組測試資料
- 加入 Benchmark 測試
- 整理效能分析結果
- 提供清楚的 Demo 使用流程

---

## 專案架構

建議專案目錄結構如下：

```text
.
busybox-diag-toolkit/
├── busybox/
│   └── BusyBox source tree or submodule
├── libdiag/
│   ├── diag_common.h
│   ├── proc_reader.c
│   ├── fs_reader.c
│   ├── net_reader.c
│   ├── formatter.c
│   └── rule_checker.c
├── applets/
│   ├── bbtop.c
│   ├── bbfscheck.c
│   └── bbnetmon.c
├── tests/
│   ├── test_bbtop.sh
│   ├── test_bbfscheck.sh
│   ├── test_bbnetmon.sh
│   └── generate_testdata.sh
├── benchmark/
│   ├── bench_bbtop.sh
│   ├── bench_bbfscheck.sh
│   ├── bench_bbnetmon.sh
│   └── results/
├── docs/
│   ├── man/
│   │   ├── bbtop.1
│   │   ├── bbfscheck.1
│   │   └── bbnetmon.1
│   ├── demo.md
│   └── benchmark.md
├── README.md
├── CONTRIBUTING.md
└── AGENTS.md
```

### 目錄說明

| 目錄 / 檔案 | 用途 |
|---|---|
| `src/` | 存放主要 `.c` 程式碼 |
| `include/` | 存放 `.h` 標頭檔 |
| `tests/` | 存放測試腳本與測試案例 |
| `benchmark/` | 存放效能測試腳本與結果 |
| `docs/` | 存放設計文件、報告、Demo 說明 |
| `data/` | 存放範例輸入資料 |
| `Makefile` | 負責編譯、清除、測試等指令 |
| `README.md` | 專案說明與協作規範 |

---

## 編譯與執行方式

### 編譯

```bash
make
```

### 執行

```bash
./main
```

或依照實際程式名稱執行：

```bash
./bin/project_name
```

### 清除編譯產物

```bash
make clean
```

### 執行測試

```bash
make test
```

### 執行 Benchmark

```bash
make benchmark
```

---

## Git Branch 使用規範

為了避免大家直接改到 `main` 導致衝突，請盡量依照以下方式開發。

### 主要分支

| Branch | 用途 |
|---|---|
| `main` | 穩定版本，只放已確認可執行的程式 |
| `develop` | 整合開發中的功能 |
| `feature/功能名稱` | 個人或小組開發新功能 |
| `fix/問題名稱` | 修正 bug |
| `docs/文件名稱` | 修改文件或報告 |

### Branch 命名範例

```bash
feature/socket-connection
feature/benchmark
feature/file-parser
fix/memory-leak
fix/makefile-error
docs/update-readme
```

---

## Git 使用流程

### 1. 開發前先更新

```bash
git checkout develop
git pull origin develop
```

### 2. 建立自己的功能分支

```bash
git checkout -b feature/your-feature-name
```

### 3. 開發完成後確認狀態

```bash
git status
```

### 4. 加入修改的檔案

```bash
git add .
```

### 5. Commit

```bash
git commit -m "feat: add basic socket connection"
```

### 6. Push 到 GitHub

```bash
git push origin feature/your-feature-name
```

### 7. 建立 Pull Request

完成後請在 GitHub 上開 Pull Request，並請至少一位組員確認後再 merge。

---

## Commit Message 規範

Commit message 建議使用以下格式：

```text
type: description
```

### 常用 type

| Type | 用途 |
|---|---|
| `feat` | 新增功能 |
| `fix` | 修正 bug |
| `docs` | 修改文件 |
| `style` | 調整排版、格式，不影響邏輯 |
| `refactor` | 重構程式碼 |
| `test` | 新增或修改測試 |
| `bench` | 新增或修改 benchmark |
| `chore` | 雜項修改，例如 Makefile、設定檔 |

### Commit 範例

```bash
git commit -m "feat(branch): add command parser"
git commit -m "fix(branch): handle empty input error"
git commit -m "docs(branch): update project structure"
git commit -m "bench(branch): add benchmark script"
git commit -m "refactor(branch): split utility functions"
```

---

## Pull Request 規範

每次開 Pull Request 時，請簡單說明這次修改了什麼。

### PR 格式

```markdown
## 修改內容

- 
- 
- 

## 測試方式

- [ ] 已成功編譯
- [ ] 已手動測試基本功能
- [ ] 已確認不會影響其他模組
- [ ] 如有需要，已更新文件

## 備註

如果有尚未完成或需要其他人協助的地方，請寫在這裡。
```

### PR 注意事項

- 不要直接 push 到 `main`
- 不要一次 PR 修改太多不相關內容
- 若修改到別人的模組，請先通知對方
- Merge 前請確認程式可以正常編譯
- 若有新增功能，請補上簡單測試方式

---

## Code Style 規範

### C 程式碼風格

本專案建議使用一致的 C code style，方便大家閱讀與維護。

### 1. 縮排

使用 4 spaces，不使用 tab。

```c
if (condition) {
    do_something();
}
```

### 2. 大括號

大括號 `{}` 採用 K&R style。

```c
void function_name(void) {
    // code
}
```

### 3. 命名規則

| 類型 | 命名方式 | 範例 |
|---|---|---|
| 變數 | snake_case | `client_fd`, `buffer_size` |
| 函式 | snake_case | `handle_client()` |
| 常數 | 全大寫 + 底線 | `MAX_BUFFER_SIZE` |
| 結構體 | snake_case | `struct client_info` |

### 4. 函式設計

每個函式應盡量只負責一件事情。

不建議：

```c
void handle_everything(void);
```

建議：

```c
void parse_input(void);
void process_request(void);
void send_response(void);
```

### 5. 錯誤處理

系統呼叫或重要函式需要檢查回傳值。

```c
int fd = open(filename, O_RDONLY);

if (fd == -1) {
    perror("open");
    return -1;
}
```

### 6. Magic Number

避免在程式中直接出現意義不明的數字。

不建議：

```c
char buffer[1024];
```

建議：

```c
#define BUFFER_SIZE 1024

char buffer[BUFFER_SIZE];
```

### 7. 註解原則

註解應說明「為什麼這樣做」，而不是只重複程式碼在做什麼。

不建議：

```c
i++; // i plus one
```

建議：

```c
// Skip the current character because it has already been processed.
i++;
```

---

## Header File 規範

`.h` 檔案應只放：

- macro 定義
- struct 定義
- enum 定義
- function prototype

範例：

```c
#ifndef CORE_H
#define CORE_H

#define BUFFER_SIZE 1024

int init_server(int port);
void handle_client(int client_fd);

#endif
```

---

## Makefile 規範

Makefile 建議至少包含以下 target：

```makefile
all:
	gcc -Wall -Wextra -I include -o main src/*.c

clean:
	rm -f main

test:
	./tests/test_basic.sh

benchmark:
	./benchmark/benchmark.sh
```

之後如果專案變大，可以再拆成更完整的編譯規則。

---

## 測試規範

測試時至少需要確認以下情況：

### 基本測試

- 正常輸入是否能得到正確輸出
- 程式是否能正常啟動與結束
- 多次執行結果是否一致

### Edge Cases

- 空輸入
- 錯誤格式輸入
- 檔案不存在
- 權限不足
- 大量資料輸入
- 非預期中斷

### 錯誤處理

- 系統呼叫失敗時是否有錯誤訊息
- 程式是否會 crash
- 是否有資源未釋放，例如 file descriptor、shared memory、semaphore 等

---

## Benchmark 規範

Benchmark 主要用來觀察程式在不同條件下的效能表現。

### 可測項目

- 執行時間
- CPU 使用率
- Memory 使用量
- I/O 次數
- 多 client / 多 process / 多 thread 情況下的表現
- 不同輸入大小下的執行效率

### Benchmark 結果紀錄

建議將結果放在：

```text
benchmark/results/
```

並用以下格式紀錄：

```markdown
# Benchmark Result

## 測試環境

- OS:
- CPU:
- Memory:
- Compiler:
- Compile flags:

## 測試方法

描述如何執行 benchmark。

## 測試結果

| 測試案例 | 輸入大小 | 執行時間 | Memory | 備註 |
|---|---:|---:|---:|---|
| case 1 | 100 | 0.01s | 1MB | 正常 |
| case 2 | 1000 | 0.08s | 2MB | 正常 |

## 分析

簡單說明效能瓶頸與可能改善方式。
```

---

## 文件規範

文件放在 `docs/` 目錄下。

建議包含：

| 文件 | 用途 |
|---|---|
| `design.md` | 系統架構、模組設計、資料流程 |
| `report.md` | 期末報告內容 |
| `demo.md` | Demo 流程與展示指令 |
| `benchmark.md` | Benchmark 設計與結果分析 |

---

## 開發注意事項

1. 每次開始寫程式前，先 `git pull`
2. 不要直接修改 `main`
3. 修改重要架構前，先和組員討論
4. Commit message 要清楚描述修改內容
5. PR 不要混入太多不相關修改
6. 程式碼要能成功編譯再 push
7. 如果新增功能，請補上簡單測試方式
8. 如果修改專案架構，請同步更新 README 或 docs

---

## TODO List

### 第一階段：專案初始化

- [ ] 建立 GitHub Repo
- [ ] 建立基本目錄架構
- [ ] 建立 Makefile
- [ ] 建立 README.md
- [ ] 確認每位成員都能 clone / push / PR

### 第二階段：核心功能開發

- [ ] 完成主要功能模組
- [ ] 完成輸入處理
- [ ] 完成輸出處理
- [ ] 完成錯誤處理
- [ ] 完成初步整合

### 第三階段：測試與修正

- [ ] 撰寫基本測試案例
- [ ] 測試 edge cases
- [ ] 修正 bug
- [ ] 檢查資源釋放問題
- [ ] 確認程式可穩定執行

### 第四階段：Benchmark 與文件

- [ ] 設計 benchmark 測試方法
- [ ] 執行 benchmark
- [ ] 整理 benchmark 結果
- [ ] 撰寫專案報告
- [ ] 整理 Demo 流程

### 第五階段：最終整合

- [ ] 完成最終版本程式
- [ ] 確認 README 完整
- [ ] 確認報告與簡報完成
- [ ] 進行 Demo 彩排
- [ ] 準備期末展示

---

## Demo 流程

Demo 時建議依照以下順序：

1. 簡介專案目標
2. 說明系統架構
3. 展示主要功能
4. 展示錯誤處理
5. 展示 Benchmark 結果
6. 說明開發中遇到的問題與解決方式
7. 總結專案成果

---

## 開發環境

建議使用以下環境：

- OS: Linux / Ubuntu
- Compiler: GCC
- Build Tool: Make
- Version Control: Git / GitHub

可使用以下指令確認環境：

```bash
gcc --version
make --version
git --version
```

---

## License

本專案僅作為課程期末專題與學習用途。
