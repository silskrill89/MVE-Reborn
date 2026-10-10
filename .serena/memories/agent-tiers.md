# Agent Tiers — MVL-clone

## Dispatch Rules
- Custom roster agents (125+ in oh-my-opencode-slim.jsonc) are NOT dispatchable via task tool
- Only 8 built-in types work: councillor, designer, explore, explorer, fixer, general, librarian, oracle
- general type LOCKED (10 consecutive Session errors)

## Route Health (re-verify after every OpenCode restart)
- WORKING: opencode-go/deepseek-v4-flash, opencode-go/deepseek-v4-pro, opencode-go/mimo-v2.5, opencode-go/mimo-v2.5-pro, opencode/mimo-v2.5-free, opencode/ling-3.0-flash-fin-free, opencode/big-pickle, openrouter/nvidia/nemotron-3-ultra-550b:free, openrouter/poolside/laguna-xs-2.1:free, openrouter/free
- DEAD: openrouter/meta-llama/llama-3.3-70b-instruct:free, opencode/deepseek-v4-flash-free, opencode/ling-3.0-tiny-free, opencode/laguna-s-2.1-free, opencode/minimax-m3-free, openrouter/minimax/minimax-m2.7:free, openrouter/z-ai/glm-4.5-air:free, all openrouter/{deepseek,moonshotai,z-ai,qwen}/* paid
- PROBE: openrouter/moonshotai/kimi-k2.6:free, openrouter/openai/gpt-oss-120b:free

## PAID CHEAP OR AUTHORIZED
- shader-fin on openrouter/inclusionai/ling-3.0-flash $0.063/M — user authorized

## Lane Timeouts
- micro (1 file ≤50 lines): ~3min
- small (1 file, logic): ~5min
- multi (2-6 files): ~8min
- heavy (RE/disasm/architecture): ~15min
