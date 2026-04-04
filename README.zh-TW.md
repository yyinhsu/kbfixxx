# kbfixxx

[English](README.md)

> *如果你也還在使用 2017 年以前版本 MacBook，被蝶式鍵盤的問題困擾，先不要放棄陪伴你多年的好夥伴～*

修復蝶式鍵盤「按一次打兩個字」問題的 macOS 選單列工具。

[Unshaky](https://github.com/aahung/Unshaky) 的增強替代方案，支援**逐鍵設定防彈跳延遲**、**多次彈跳抑制**、**自動偵測問題按鍵**，設定檔為可直接編輯的 JSON 格式。

## 功能特色

- **逐鍵防彈跳延遲** — 每個按鍵可設定不同的延遲門檻 (毫秒)
- **多次彈跳抑制** — 處理連續彈跳 2–3 次以上的按鍵，不只抑制一次
- **自動偵測** — 自動辨識頻繁重複觸發的按鍵，並建議適當延遲值
- **即時事件日誌** — 查看每次按鍵事件（被抑制或放行），含時間戳記
- **統計面板** — 各按鍵的抑制次數與彈跳率
- **JSON 設定檔** — 可直接編輯的設定檔 `~/.config/kbfixxx/config.json`，儲存後自動重新載入
- **選單列常駐** — 僅顯示於選單列，不佔用 Dock 空間

## 安裝方式

### 下載安裝（推薦）

1. 前往 [Releases](../../releases/latest) 頁面
2. 下載 `kbfixxx.app.zip`
3. 解壓縮後將 `kbfixxx.app` 移至 `/Applications`
4. **首次啟動**：對 App 按右鍵 → **打開** → 再次點擊 **打開**（macOS 預設會阻擋未簽署的 App）。或在終端機執行：`xattr -cr /Applications/kbfixxx.app`
   > ⚠️ 請在信任本專案的前提下執行此操作。本專案完全開源，你可以自行檢視原始碼或[從原始碼編譯](#從原始碼編譯)。
5. 依提示授予「**輔助使用**」及「**輸入監視**」權限（詳見[疑難排解：輔助使用權限](#疑難排解輔助使用權限)）

### 從原始碼編譯

```sh
# 安裝 Xcode 命令列工具（若尚未安裝）
xcode-select --install

# 複製專案
git clone https://github.com/user/kbfixxx.git
cd kbfixxx

# 編譯 App Bundle
make

# （選用）安裝預設設定並啟動
make run
```

編譯完成後，將 `kbfixxx.app` 移至 `/Applications`，首次啟動時依提示授予「**輔助使用**」及「**輸入監視**」權限即可（詳見[疑難排解：輔助使用權限](#疑難排解輔助使用權限)）。

## 系統需求

- macOS 10.13 (High Sierra) 或更新版本
- Xcode 命令列工具 (`xcode-select --install`)
- **輔助使用權限**（系統設定 → 隱私與安全性 → 輔助使用）
- **輸入監視權限**（系統設定 → 隱私與安全性 → 輸入監視）

## 編譯與執行

```sh
# 編譯 App Bundle
make

# 編譯並啟動
make run

# 執行單元測試
make test

# 清除編譯產物
make clean
```

App 會以鍵盤圖示出現在選單列。首次啟動時會要求「輔助使用」權限——這是攔截鍵盤事件的必要權限。

## 疑難排解：輔助使用權限

kbfixxx 使用 `CGEventTap` 攔截鍵盤事件。macOS 要求同時具備ㄏ才能正常運作。

### Event Tap 未啟用 / 統計和事件日誌為空

如果選單列顯示 **「Event Tap: Inactive ⚠️」**，或統計面板和事件日誌視窗完全是空的，代表事件攔截已失去權限。常見原因：

- **更新 App（下載新版本或重新編譯）** — 當 binary 的 hash 改變時，macOS 會自動撤銷權限
- **macOS 系統更新**

**修復步驟：**

1. 結束 kbfixxx（選單列 → Quit kbfixxx）
2. 打開 **系統設定 → 隱私與安全性 → 輔助使用**
3. 在清單中找到 kbfixxx，**移除它**（點擊 `−` 按鈕）
4. **重新加入** kbfixxx（點擊 `+` 按鈕，選擇 `/Applications/kbfixxx.app`）
5. 打開 **系統設定 → 隱私與安全性 → 輸入監視**
6. 同樣操作：**移除** kbfixxx，然後**重新加入**
7. 重新啟動 kbfixxx

> ⚠️ 單純把開關關掉再打開是**不夠的** — 必須移除後重新加入，macOS 才會重新讀取新的 binary hash。

重新啟動後，確認選單列顯示 **「Event Tap: Active」**，並且打開事件日誌視窗後打字能看到事件記錄。

## 設定

設定檔位置：`~/.config/kbfixxx/config.json`

```json
{
    "global": {
        "ignore_external_keyboard": false,
        "ignore_internal_keyboard": false
    },
    "keys": {
        "b":     { "delay_ms": 60, "max_bounce_count": 1, "enabled": true },
        "space": { "delay_ms": 80, "max_bounce_count": 2, "enabled": true },
        "t":     { "delay_ms": 40, "max_bounce_count": 1, "enabled": true }
    }
}
```

### 逐鍵設定

| 欄位 | 說明 | 範圍 |
|------|------|------|
| `delay_ms` | 防彈跳時間窗口（毫秒）。在 keyUp 之後的這段時間內，若再次偵測到 keyDown，則視為彈跳。 | 0–2000 |
| `max_bounce_count` | 連續彈跳的最大抑制次數。如果某個鍵會連續彈跳多次，設為 2 以上。 | 1–10 |
| `enabled` | 是否啟用此按鍵的防彈跳功能。 | true/false |

按鍵可用名稱指定（`b`、`space`、`return`、`tab` 等），也可用數字鍵碼（`11`、`49`）。

### 如何選擇適當延遲

從 **40 ms** 開始，如果彈跳仍會漏過，再逐步增加。打字速度快的人（例如快速輸入 "apple"、"coffee"）若設太長的延遲，可能會誤擋正常的連續相同字母。一般建議範圍：**40–80 ms**。

### 全域設定

| 欄位 | 說明 |
|------|------|
| `ignore_external_keyboard` | 不對外接鍵盤做防彈跳處理 |
| `ignore_internal_keyboard` | 不對內建鍵盤做防彈跳處理 |

## 運作原理

```
實體按鍵按下
    │
    ▼
CGEventTap 攔截 keyDown/keyUp 事件
    │
    ▼
此按鍵是否有設定防彈跳？
    │ 否 → 放行
    ▼ 是
這是 keyDown 事件嗎？
    │ 否（keyUp） → 放行（記錄時間戳記）
    ▼ 是
是否為長按重複（auto-repeat）？
    │ 是 → 放行
    ▼ 否
距上次 keyUp < delay_ms？
  或距上次 keyDown < delay_ms？
    │ 否 → 放行（正常按鍵）
    ▼ 是
抑制事件（回傳 NULL）
```

App 使用 `CGEventTapCreate` 在 session 層級攔截鍵盤事件，事件會在送到應用程式之前被處理。當偵測到彈跳 — 非長按重複的 keyDown 在上次 keyUp 或上次 keyDown 的 `delay_ms` 以內出現 — 該事件會被抑制。所有 keyUp 事件無條件放行，以維持正確的時間基準。

## 選單列

點擊選單列的鍵盤圖示可存取：

- **啟用/停用** — 切換防彈跳功能的開關
- **設定…** — 開啟逐鍵設定視窗（含「偵測按鍵」按鈕）
- **統計** — 檢視各按鍵的抑制次數及自動偵測到的彈跳按鍵
- **事件日誌** — 即時滾動的鍵盤事件記錄
- **權限狀態** — 顯示「輔助使用」與「輸入監視」的授權狀態；點擊可快速開啟系統偏好設定
- **重新載入設定** — 手動重新載入 JSON 設定檔
- **開啟設定檔** — 用預設編輯器開啟 `config.json`

## 架構

```
src/
├── core/           # 純 C — 不依賴 Cocoa
│   ├── debouncer   # CGEventTap 過濾器，含多次彈跳邏輯
│   ├── config      # 透過 cJSON 載入/儲存 JSON 設定
│   ├── keymap      # 146 個 macOS 虛擬鍵碼 ↔ 名稱對應
│   ├── detector    # 自動偵測彈跳按鍵
│   └── stats       # 逐鍵事件/抑制計數器
├── app/            # Objective-C — Cocoa UI
│   ├── AppDelegate # 選單列、事件攔截生命週期、FSEvents 監控
│   ├── Preference  # 逐鍵設定表格，含偵測按鍵功能
│   ├── Stats       # 統計面板 + 自動偵測建議
│   └── Log         # 即時滾動事件日誌
└── vendor/
    └── cJSON       # JSON 解析器（MIT 授權）
```

## 授權條款

MIT
