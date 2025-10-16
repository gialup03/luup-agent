#!/usr/bin/env python3
"""
Basic usage test for Python bindings.
"""

import sys
from pathlib import Path

# Add package to path
sys.path.insert(0, str(Path(__file__).parent))

from luup_agent import Model, Agent

print("Testing luup-agent Python bindings...")
print()

# Find model file
model_path = Path(__file__).parent.parent.parent / "models" / "qwen2-0.5b-instruct-q4_k_m.gguf"

if not model_path.exists():
    print(f"⚠ Model file not found: {model_path}")
    print("Skipping runtime test (but imports work!)")
    sys.exit(0)

print(f"✓ Model file found: {model_path}")
print()

# Test 1: Create model
print("Test 1: Creating model...")
try:
    model = Model.from_local(
        str(model_path),
        gpu_layers=-1,
        context_size=512,
        threads=1,
    )
    print("✓ Model created successfully")
except Exception as e:
    print(f"✗ Failed: {e}")
    sys.exit(1)

# Test 2: Get model info
print("\nTest 2: Getting model info...")
try:
    info = model.get_info()
    print(f"✓ Backend: {info['backend']}")
    print(f"✓ Device: {info['device']}")
    print(f"✓ Context size: {info['context_size']}")
    print(f"✓ GPU layers: {info['gpu_layers_loaded']}")
except Exception as e:
    print(f"✗ Failed: {e}")
    sys.exit(1)

# Test 3: Create agent
print("\nTest 3: Creating agent...")
try:
    agent = Agent(
        model,
        system_prompt="You are a helpful assistant. Be very concise.",
        temperature=0.7,
        max_tokens=50,
        enable_tool_calling=True,
        enable_history=True,
    )
    print("✓ Agent created successfully")
except Exception as e:
    print(f"✗ Failed: {e}")
    sys.exit(1)

# Test 4: Register a tool
print("\nTest 4: Registering tool...")
try:
    @agent.tool(description="Add two numbers")
    def add(a: int, b: int) -> int:
        """Add two numbers together."""
        return a + b
    
    print("✓ Tool registered successfully")
    print(f"✓ Tools: {list(agent._tools.keys())}")
except Exception as e:
    print(f"✗ Failed: {e}")
    sys.exit(1)

# Test 5: Generate response
print("\nTest 5: Generating response...")
try:
    response = agent.generate("Say hello in exactly 3 words")
    print(f"✓ Response: {response}")
except Exception as e:
    print(f"✗ Failed: {e}")
    sys.exit(1)

# Test 6: History
print("\nTest 6: Checking history...")
try:
    history = agent.get_history()
    print(f"✓ History has {len(history)} messages")
    if len(history) >= 2:
        print(f"✓ Last user message: {history[-2]['content'][:50]}...")
        print(f"✓ Last assistant message: {history[-1]['content'][:50]}...")
except Exception as e:
    print(f"✗ Failed: {e}")
    sys.exit(1)

# Cleanup
print("\nCleaning up...")
agent.close()
model.close()

print()
print("=" * 70)
print("✅ All tests passed! Python bindings are working correctly.")
print("=" * 70)

