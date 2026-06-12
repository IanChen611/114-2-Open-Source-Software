# TODO LIST — 114-2 開源系統軟體與實務 期末專題

> 工作分配：**陳翊揚（0.5 份）**、**同學 A（1 份）**、**同學 B（1 份）**

---

## 陳翊揚（0.5 份）

### 程式碼清理（已完成）
- [x] 重新命名三個驗證函式，移除 `_placeholder` 後綴
- [x] 移除過時的 TODO 註解（main.c:24–27, main.c:48–50）

### 核心模組（已完成）
- [x] 建立 `sudoku_module.c`，實作可載入的 Linux kernel module（`module_init` / `module_exit`）
- [x] 在核心模組中實作字元裝置 `/dev/sudoku`，支援 `read`、`write`、`ioctl`
- [x] 將驗證邏輯（行、列、宮格）移植到核心空間版本（使用 `pr_info` / `pr_err`，不使用 `printf`）
- [x] 實作 `ioctl` 指令：`NEW_GAME`、`RESET`、`HINT`

### CLI 客戶端（已完成）
- [x] 建立 `sudoku-cli.c`，透過 `open("/dev/sudoku")` 與核心模組溝通
- [x] 實作棋盤渲染（9×9 格線、數字顯示）
- [x] 實作互動式落子輸入（格式 `列 欄 數值`，1-indexed）

### 建置系統（已完成）
- [x] 建立 `Makefile`，分為兩個目標：
  - `make` — 編譯核心模組（`obj-m := sudoku_module.o`）
  - `make userspace` — 編譯 CLI 客戶端及 `main.c` 驗證器

### Git 設定（進行中）
- [x] 建立 `.gitignore`，排除 `*.ko`、`*.o`、`*.mod.c`、`Module.symvers`、`modules.order` 等核心建置產物

### 期末簡報（待完成）
- [ ] 撰寫好 README.md 
- [ ] 負責自己部分的投影片（現有程式碼架構、字元裝置介面、核心 API 使用方式）

---

## 同學 A（1 份）

### 核心模組（待完成）
- [x] 實作核心空間的謎題生成器（使用 `get_random_u32()` 取代 `rand()`，回溯法填滿棋盤後隨機移除 40 格）

### 最終整合測試（待完成）
- [x] 在乾淨的 WSL2 環境上從零執行全部流程，確認整體功能正常
  - [x] 新增 `make prepare-wsl2-kernel`，自動安裝依賴、clone 對應 WSL2 kernel branch，並建立可編外部 module 的 build tree
  - [x] 新增 `make integration-test`，自動執行 userspace build、kernel module build、載入 `/dev/sudoku`、CLI smoke test 與卸載
  - [x] 於可登入的乾淨 WSL2 Ubuntu 環境執行並記錄結果

### 期末簡報（待完成）
- [ ] 負責自己部分的投影片（謎題生成器演算法、整合測試結果）

---

## 同學 B（1 份）

### 建置系統（待完成）
- [ ] 加入 `make test` 目標，在 userspace 執行驗證邏輯的單元測試

### Git 設定（待完成）
- [ ] 建立 `.git/hooks/pre-commit`，提交前自動執行 `make test`

### 期末簡報（待完成）
- [ ] 負責自己部分的投影片（建置系統、Git Hook 說明、測試框架）
