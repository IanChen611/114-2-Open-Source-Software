# TODO LIST — 114-2 開源系統軟體與實務 期末專題

> 工作分配：**陳翊揚（0.5 份）**、**同學 A（1 份）**、**同學 B（1 份）**

---

### 程式碼清理（已完成）
- [x] 重新命名三個驗證函式，移除 `_placeholder` 後綴
- [x] 移除過時的 TODO 註解（main.c:24–27, main.c:48–50）

### Git 設定（進行中）
- [x] 建立 `.gitignore`，排除 `*.ko`、`*.o`、`*.mod.c`、`Module.symvers`、`modules.order` 等核心建置產物
- [ ] 建立 `.git/hooks/commit-msg`，強制驗證訊息格式（需包含 `[Feature Addition]`、`[Bug Fixing]`、`[Refactoring]`、`[Document]` 等 tag）

### 建置系統
- [ ] 加入 `make test` 目標，在 userspace 執行驗證邏輯的單元測試

---

## 同學 A（1 份）

### 核心模組（已完成）
- [x] 建立 `sudoku_module.c`，實作可載入的 Linux kernel module（`module_init` / `module_exit`）
- [x] 在核心模組中實作字元裝置 `/dev/sudoku`，支援 `read`、`write`、`ioctl`
- [x] 將驗證邏輯（行、列、宮格）移植到核心空間版本（使用 `pr_info` / `pr_err`，不使用 `printf`）
- [x] 實作 `ioctl` 指令：`NEW_GAME`、`RESET`、`HINT`

### 核心模組（待完成）
- [ ] 實作核心空間的謎題生成器（使用 `get_random_u32()` 取代 `rand()`，回溯法填滿棋盤後隨機移除 40 格）

---

## 同學 B（1 份）

### CLI 客戶端（已完成）
- [x] 建立 `sudoku-cli.c`，透過 `open("/dev/sudoku")` 與核心模組溝通
- [x] 實作棋盤渲染（9×9 格線、數字顯示）
- [x] 實作互動式落子輸入（格式 `列 欄 數值`，1-indexed）

### 建置系統（已完成）
- [x] 建立 `Makefile`，分為兩個目標：
  - `make` — 編譯核心模組（`obj-m := sudoku_module.o`）
  - `make userspace` — 編譯 CLI 客戶端及 `main.c` 驗證器

### Git 設定（待完成）
- [ ] 建立 `.git/hooks/pre-commit`，提交前自動執行 `make test`

### 期末報告（待完成）
- [ ] 撰寫期末報告（Word 格式，4 頁，800 字以上），內容涵蓋：
  - 系統架構說明（kernel module ↔ `/dev/sudoku` ↔ CLI）
  - 核心 API 使用方式（`cdev`、`ioctl`、`copy_to_user` 等）
  - Git 工作流程與 Hook 說明
  - 成果展示截圖

---

## 共同（全員）

- [ ] 最終整合測試（在乾淨的 WSL2 環境上從零執行全部流程）
- [ ] 期末簡報製作（參考 Zuvio 上的範例格式）
