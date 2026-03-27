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

## 系統需求

- macOS 12.0 (Monterey) 或更新版本
- Xcode 命令列工具 (`xcode-select --install`)
- **輔助使用權限**（系統設定 → 隱私與安全性 → 輔助使用）

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
前一個 keyUp 距今是否在 delay_ms 以內？
    │ 否 → 放行（重設彈跳計數器）
    ▼ 是
遞增 bounce_counter
    │
    ▼
bounce_counter ≤ max_bounce_count？
    │ 否 → 放行（使用者確實在快速輸入）
    ▼ 是
抑制事件（回傳 NULL）
```

App 使用 `CGEventTapCreate` 在 session 層級攔截鍵盤事件，事件會在送到應用程式之前被處理。當偵測到彈跳（keyUp 後短時間內又出現 keyDown），該事件會被抑制，對應的 keyUp 也會一併丟棄。

## 選單列

點擊選單列的鍵盤圖示可存取：

- **啟用/停用** — 切換防彈跳功能的開關
- **設定…** — 開啟逐鍵設定視窗（含「偵測按鍵」按鈕）
- **統計** — 檢視各按鍵的抑制次數及自動偵測到的彈跳按鍵
- **事件日誌** — 即時滾動的鍵盤事件記錄
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
