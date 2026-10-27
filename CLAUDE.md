# aramakisai-parking-cam

## プロジェクト概要

荒牧祭の駐車場の混雑状況を自動更新するための、ESP32-CAM向け端末ファームウェア。駐車場を一定間隔で撮影し、JPEGをサーバーへHTTPSでPOSTする。受信・混雑判定・公開ページ`/parking`の表示は姉妹リポジトリ`aramakisai-web`で実装し、本リポジトリで実装するのは端末ファームウェアだけ。

## コマンド

PlatformIOはPATHに入っていないため、フルパスで呼ぶ。

```bash
~/.platformio/penv/bin/pio run                 # ビルド
~/.platformio/penv/bin/pio run -t upload       # 書込み(CH340の自動リセットで書ける。GPIO0のジャンパーは不要)
~/.platformio/penv/bin/pio device monitor      # シリアルモニタ(115200bps)
```

## 注意事項

- 特別な指示がない限り、応答・コミットメッセージを含めてすべて日本語を使う
- 送信先URL・デバイス用トークン・WiFi資格情報などの設定の実値はコミットしない。雛形だけをコミットする
- 本リポジトリのspec(`parking-camera-firmware`)は端末ファームウェアだけを扱う。CMSの受信エンドポイントと公開ページのspecは`aramakisai-web`の`parking-camera-endpoint`にある

# Agentic SDLC and Spec-Driven Development

Kiro-style Spec-Driven Development on an agentic SDLC

## Project Context

### Paths
- Steering: `.kiro/steering/`
- Specs: `.kiro/specs/`

### Steering vs Specification

**Steering** (`.kiro/steering/`) - Guide AI with project-wide rules and context
**Specs** (`.kiro/specs/`) - Formalize development process for individual features

### Active Specifications
- Check `.kiro/specs/` for active specifications
- Use `/kiro:spec-status [feature-name]` to check progress

## Development Guidelines
- Think in English, generate responses in Japanese. All Markdown content written to project files (e.g., requirements.md, design.md, tasks.md, research.md, validation reports) MUST be written in the target language configured for this specification (see spec.json.language).

## Minimal Workflow
- Phase 0 (optional): `/kiro:steering`, `/kiro:steering-custom`
- Phase 1 (Specification):
  - `/kiro:spec-init "description"`
  - `/kiro:spec-requirements {feature}`
  - `/kiro:validate-gap {feature}` (optional: for existing codebase)
  - `/kiro:spec-design {feature} [-y]`
  - `/kiro:validate-design {feature}` (optional: design review)
  - `/kiro:spec-tasks {feature} [-y]`
- Phase 2 (Implementation): `/kiro:spec-impl {feature} [tasks]`
  - `/kiro:validate-impl {feature}` (optional: after implementation)
- Progress check: `/kiro:spec-status {feature}` (use anytime)

## Development Rules
- 3-phase approval workflow: Requirements → Design → Tasks → Implementation
- specは使い捨てのすり合わせ資料。実装後に追随更新しない
- Human review required each phase; use `-y` only for intentional fast-track
- Keep steering current and verify alignment with `/kiro:spec-status`
- Follow the user's instructions precisely, and within that scope act autonomously: gather the necessary context and complete the requested work end-to-end in this run, asking questions only when essential information is missing or the instructions are critically ambiguous.

## Steering Configuration
- Load entire `.kiro/steering/` as project memory
- Default files: `product.md`, `tech.md`, `structure.md`
- Custom files are supported (managed via `/kiro:steering-custom`)
