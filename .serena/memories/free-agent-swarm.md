# Free Agent Swarm — MVL-clone

## Free Models Available on OpenRouter
- opencode/inclusionai/ling-3.0-flash:free (128K)
- openrouter/moonshotai/kimi-k2.6:free (131K)
- openrouter/openai/gpt-oss-120b:free (131K)
- openrouter/poolside/laguna-xs-2.1:free (262K)
- openrouter/nvidia/nemotron-3-ultra-550b-a55b:free (131K)
- nemotron-3.5-llama-3.5-8b-lightning:free (131K)
- nemotron-reasoning-super-120b:free (128K)

## Additional Free Models Discovered
- deepseek/deepseek-r1-0528:free, qwen/qwen3-235b-a22b:free, qwen/qwen3-coder:free
- google/gemma-3-27b-it:free, mistralai/devstral-small:free
- nvidia/llama-3.1-nemotron-ultra-253b-v1:free
- z-ai/mimo-v2-pro:free, z-ai/mimo-v2:free, z-ai/glm-4.7:free, z-ai/glm-4.8:free, z-ai/glm-4.9:free
- minimax-m1-80k:free, volcengine-doubao-1-5-pro-256k:free
- stepfun/step-3.5-flash:free, stepfun/step-3-flash-128k:free
- qwen/qwen3-30b-a3b:free, tencent-hunyuan/hunyuan-a13b-instruct:free
- bytedance-seed/seed-1.6-flash:free, bytedance-seed/seed-1.6:free

## Agent Role Mapping
| Role | Best Free Model | Why |
|------|----------------|-----|
| crash RE | nemotron-reasoning-super-120b:free | Strong reasoning |
| buffer audit | deepseek-r1-0528:free | Thorough analysis |
| envmap audit | kimi-k2.6:free | Large context |
| TODO audit | qwen3-235b-a22b:free | Comprehensive |
| code review | gpt-oss-120b:free | Strong review |
| doc update | ling-3.0-flash:free | Fast + cheap |

## Dispatch Strategy
- Dispatch 2-3 lanes max concurrently (credit limit)
- Each lane 8-15 minutes budget
- Use built-in agent types (oracle/explorer/explore/fixer)
