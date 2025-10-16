#!/usr/bin/env python3
"""
Remote API Backend Demo

Demonstrates using luup-agent with OpenAI-compatible remote APIs:
- Creating remote models with API endpoints
- Basic text generation
- Streaming responses
- Custom endpoints (Ollama, OpenRouter, etc.)

Usage:
    export OPENAI_API_KEY="sk-..."
    python remote_api.py

Or with custom endpoint:
    export API_ENDPOINT="https://api.openrouter.ai/api/v1"
    export API_KEY="sk-..."
    python remote_api.py
"""

import os
import sys
from pathlib import Path

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent))

from luup_agent import Model, Agent


def main():
    print("=== luup-agent Remote API Demo (Python) ===\n")
    
    # Get API key from environment
    api_key = os.getenv("OPENAI_API_KEY") or os.getenv("API_KEY")
    if not api_key:
        print("Error: Please set OPENAI_API_KEY or API_KEY environment variable")
        print('Example: export OPENAI_API_KEY="sk-..."')
        return 1
    
    # Get custom endpoint from environment (optional)
    api_endpoint = os.getenv("API_ENDPOINT", "https://api.openai.com/v1")
    
    print(f"API Endpoint: {api_endpoint}")
    print(f"API Key: {api_key[:10]}...\n")
    
    # =========================================================================
    # Example 1: Create Remote Model
    # =========================================================================
    print("--- Example 1: Creating Remote Model ---")
    
    try:
        # Using context manager for automatic cleanup
        with Model.from_remote(
            endpoint=api_endpoint,
            api_key=api_key,
            context_size=4096
        ) as model:
            print("✓ Remote model created successfully\n")
            
            # Get model info
            info = model.get_info()
            print("Model Information:")
            print(f"  Backend:       {info['backend']}")
            print(f"  Device:        {info['device']}")
            print(f"  Context Size:  {info['context_size']} tokens")
            print()
            
            # =====================================================================
            # Example 2: Basic Chat Agent
            # =====================================================================
            print("--- Example 2: Basic Chat Agent ---")
            
            with Agent(
                model=model,
                system_prompt="You are a helpful AI assistant.",
                temperature=0.7,
                max_tokens=150,
                enable_builtin_tools=False  # Disable built-in tools for remote demo
            ) as agent:
                print("✓ Agent created successfully\n")
                
                # Generate response
                print("User: Tell me a short joke about programming.")
                print("Assistant: ", end="", flush=True)
                
                response = agent.generate("Tell me a short joke about programming.")
                print(response + "\n")
                
                # =================================================================
                # Example 3: Streaming Generation
                # =================================================================
                print("--- Example 3: Streaming Generation ---")
                
                print("User: Write a haiku about artificial intelligence.")
                print("Assistant: ", end="", flush=True)
                
                # Collect streamed tokens
                tokens = []
                for token in agent.generate_stream("Write a haiku about artificial intelligence."):
                    print(token, end="", flush=True)
                    tokens.append(token)
                
                print("\n\n✓ Streaming completed successfully\n")
                
                # =================================================================
                # Example 4: Multi-turn Conversation
                # =================================================================
                print("--- Example 4: Multi-turn Conversation ---")
                
                # First turn
                print("User: What is the capital of France?")
                print("Assistant: ", end="", flush=True)
                
                response = agent.generate("What is the capital of France?")
                print(response + "\n")
                
                # Second turn (context is maintained)
                print("User: What is its population?")
                print("Assistant: ", end="", flush=True)
                
                response = agent.generate("What is its population?")
                print(response + "\n")
                
                # Get conversation history
                history = agent.get_history()
                print(f"✓ Conversation has {len(history)} messages\n")
                
                # =================================================================
                # Example 5: Custom Endpoint (Ollama Local Server)
                # =================================================================
                print("--- Example 5: Custom Endpoint Support ---")
                print("Note: For Ollama or other custom endpoints, set:")
                print('  export API_ENDPOINT="http://localhost:11434/v1"')
                print('  export API_KEY="ollama"  # Any value works for local Ollama')
                print("  python remote_api.py\n")
    
    except Exception as e:
        print(f"Error: {e}")
        return 1
    
    print("--- Cleanup ---")
    print("✓ Resources cleaned up automatically (context managers)")
    print("\n=== Demo Complete ===")
    return 0


if __name__ == "__main__":
    sys.exit(main())

