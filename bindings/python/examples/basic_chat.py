#!/usr/bin/env python3
"""
Basic chat example using luup-agent Python bindings.

This example demonstrates:
- Creating a local model with GPU acceleration
- Creating an agent with a system prompt
- Simple chat loop with conversation history
"""

import sys
from pathlib import Path

from luup_agent import Model, Agent


def main():
    print("luup-agent Python Basic Chat Example")
    print("=" * 50)
    
    # Check for model path argument
    if len(sys.argv) < 2:
        print("Usage: python basic_chat.py <path-to-model.gguf>")
        print("Example: python basic_chat.py ../../models/qwen2-0.5b-instruct-q4_k_m.gguf")
        return 1
    
    model_path = sys.argv[1]
    
    if not Path(model_path).exists():
        print(f"Error: Model file not found: {model_path}")
        return 1
    
    print(f"\nCreating model from: {model_path}")
    
    # Create model with GPU acceleration
    model = Model.from_local(
        model_path,
        gpu_layers=-1,      # Auto-detect and use all available GPU
        context_size=2048,
        threads=0,          # Auto-detect CPU threads
    )
    
    print("Model created successfully!")
    
    # Get and display model info
    info = model.get_info()
    print(f"Backend: {info['backend']}")
    print(f"Device: {info['device']}")
    print(f"Context size: {info['context_size']}")
    print(f"GPU layers loaded: {info['gpu_layers_loaded']}")
    
    # Warmup model for better first-token latency
    print("\nWarming up model...")
    model.warmup()
    
    # Create agent
    print("Creating agent...")
    agent = Agent(
        model,
        system_prompt="You are a helpful AI assistant. Be concise and friendly.",
        temperature=0.7,
        max_tokens=512,
        enable_tool_calling=False,  # No tools for basic chat
        enable_history=True
    )
    
    print("Agent created successfully!\n")
    print("Chat started! Type 'quit' or 'exit' to end.")
    print("=" * 50)
    print()
    
    # Simple chat loop
    try:
        while True:
            # Get user input
            user_input = input("You: ")
            
            # Check for quit
            if user_input.lower() in ["quit", "exit", ""]:
                break
            
            # Generate response
            print("Assistant: ", end='', flush=True)
            
            try:
                response = agent.generate(user_input)
                print(response)
                print()
            except Exception as e:
                print(f"\nError generating response: {e}\n")
    
    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
    
    finally:
        print("\nGoodbye!")
        
        # Cleanup (optional, will be done automatically)
        agent.close()
        model.close()
    
    return 0


if __name__ == "__main__":
    sys.exit(main())

