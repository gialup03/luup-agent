#!/usr/bin/env python3
"""
Async streaming example using luup-agent Python bindings.

This example demonstrates:
- Asynchronous generation with async/await
- Token-by-token streaming for real-time output
- Using asyncio for concurrent operations
"""

import sys
import asyncio
from pathlib import Path

from luup_agent import Model, Agent


async def stream_response(agent: Agent, message: str) -> str:
    """
    Generate and stream a response asynchronously.
    
    Args:
        agent: Agent instance
        message: User message
        
    Returns:
        Full generated response
    """
    print("Assistant: ", end='', flush=True)
    
    tokens = []
    async for token in agent.generate_async(message):
        print(token, end='', flush=True)
        tokens.append(token)
    
    print()  # Newline after response
    
    return ''.join(tokens)


async def main():
    print("luup-agent Python Async Streaming Example")
    print("=" * 50)
    
    # Check for model path
    if len(sys.argv) < 2:
        print("Usage: python async_streaming.py <path-to-model.gguf>")
        print("Example: python async_streaming.py ../../models/qwen2-0.5b-instruct-q4_k_m.gguf")
        return 1
    
    model_path = sys.argv[1]
    
    if not Path(model_path).exists():
        print(f"Error: Model file not found: {model_path}")
        return 1
    
    print(f"\nCreating model from: {model_path}")
    
    # Create model
    model = Model.from_local(
        model_path,
        gpu_layers=-1,
        context_size=2048,
        threads=0,
    )
    
    print("Model created successfully!")
    
    # Create agent
    agent = Agent(
        model,
        system_prompt="You are a creative storyteller. Write engaging short stories.",
        temperature=0.8,  # Higher temperature for more creative output
        max_tokens=512,
        enable_tool_calling=False,
        enable_history=True
    )
    
    print("Agent created successfully!\n")
    print("=" * 50)
    print("Streaming responses in real-time...")
    print("=" * 50)
    print()
    
    # Example 1: Single streaming request
    print("Example 1: Short story")
    print("-" * 50)
    await stream_response(agent, "Tell me a very short story about a robot.")
    print()
    
    # Example 2: Multiple concurrent requests (demonstrates async capability)
    print("\nExample 2: Multiple questions")
    print("-" * 50)
    
    questions = [
        "What is the meaning of life in one sentence?",
        "Describe the color blue to someone who has never seen it.",
    ]
    
    for question in questions:
        print(f"\nYou: {question}")
        await stream_response(agent, question)
    
    print()
    
    # Example 3: Interactive streaming
    print("\n" + "=" * 50)
    print("Interactive streaming mode")
    print("Type your messages (or 'quit' to exit)")
    print("=" * 50)
    print()
    
    try:
        while True:
            # Get user input (in async context)
            user_input = await asyncio.to_thread(input, "You: ")
            
            if user_input.lower() in ["quit", "exit", ""]:
                break
            
            # Stream response
            await stream_response(agent, user_input)
            print()
    
    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
    
    finally:
        print("\nExample complete!")
        agent.close()
        model.close()
    
    return 0


def run():
    """Run the async main function."""
    try:
        return asyncio.run(main())
    except KeyboardInterrupt:
        return 0


if __name__ == "__main__":
    sys.exit(run())

