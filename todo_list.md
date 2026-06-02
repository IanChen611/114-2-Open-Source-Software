# TODO LIST — main.c 及整體專題

## 程式碼清理

- [x] **重新命名三個驗證函式**，移除 `_placeholder` 後綴（`validate_row_placeholder` → `validate_row`，以此類推），並同步更新 `worker()` 中的呼叫點（main.c:28, main.c:51, main.c:73）
- [x] **移除過時的 TODO 註解**（main.c:24–27, main.c:48–50），邏輯已實作完成，這些 comment 會誤導讀者

---

## 功能補強（userspace）

- [x] **支援從檔案或 stdin 讀入棋盤**，取代 main.c:125–135 的硬編碼棋盤，讓程式可驗證任意輸入
- [x] **`worker()` 中的 `printf` 會造成輸出亂序**（多執行緒同時寫 stdout），考慮改用 mutex 保護，或在 `pthread_join` 之後統一輸出結果（main.c:110–119）
- [x] **錯誤處理**：`pthread_create` 失敗時已建立的執行緒未被 join，應在 return 前清理（main.c:151–155）

---

## 核心模組（新增）

- [ ] 建立 `sudoku_module.c`，實作可載入的 Linux kernel module（`module_init` / `module_exit`）
- [ ] 在核心模組中實作字元裝置 `/dev/sudoku`，支援 `read`、`write`、`ioctl`
- [ ] 將驗證邏輯（行、列、宮格）移植到核心空間版本（不能使用 `printf`，改用 `pr_info` / `pr_err`）
- [ ] 實作 `ioctl` 指令：`NEW_GAME`、`RESET`、`HINT`
- [ ] 實作核心空間的謎題生成器（使用 `get_random_u32()` 取代 `rand()`）

---

## CLI 客戶端（新增）

- [ ] 建立 `sudoku-cli.c`，透過 `open("/dev/sudoku")` 與核心模組溝通
- [ ] 實作棋盤渲染（可重用或參考 `print_board()`）
- [ ] 實作互動式落子輸入，格式為 `列 欄 數值`

---

## 建置系統（新增）

- [ ] 建立 `Makefile`，分為兩個目標：
  - `make` — 編譯核心模組（`obj-m := sudoku_module.o`）
  - `make userspace` — 編譯 CLI 客戶端及現有的 `main.c` 驗證器
- [ ] 加入 `make test` 目標，在 userspace 執行驗證邏輯的單元測試

---

## Git 設定（新增）

- [ ] 建立 `.git/hooks/pre-commit`，提交前自動執行 `make test`
- [ ] 建立 `.git/hooks/commit-msg`，強制驗證訊息格式（`[Feature Addition]`、`[Bug Fixing]` 等）
- [ ] 建立 `.gitignore`，排除 `*.ko`、`*.o`、`*.mod.c`、`Module.symvers`、`modules.order` 等核心建置產物
