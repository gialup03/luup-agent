#!/usr/bin/env python3
"""
Tool calling example using luup-agent Python bindings.

This example demonstrates:
- Registering custom tools with the @agent.tool() decorator
- Automatic tool invocation by the agent
- Type hints for parameter schema generation
"""

import sys
from pathlib import Path
from typing import Optional

from luup_agent import Model, Agent


def main():
    print("luup-agent Python Tool Calling Example")
    print("=" * 50)
    
    # Check for model path
    if len(sys.argv) < 2:
        print("Usage: python tool_calling.py <path-to-model.gguf>")
        print("Example: python tool_calling.py ../../models/qwen2-0.5b-instruct-q4_k_m.gguf")
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
    
    # Create agent with tool calling enabled
    agent = Agent(
        model,
        system_prompt=(
            "You are a helpful assistant with access to tools. "
            "Use the get_weather tool to check weather and "
            "the calculate tool for math operations. "
            "Always use tools when appropriate."
        ),
        temperature=0.7,
        max_tokens=512,
        enable_tool_calling=True,
        enable_history=True
    )
    
    print("Agent created successfully!\n")
    
    # Register weather tool
    @agent.tool(description="Get current weather for a city")
    def get_weather(city: str, units: str = "celsius") -> dict:
        """Get current weather information for a city."""
        print(f"  [Tool called: get_weather(city='{city}', units='{units}')]")
        
        # In a real implementation, you would call a weather API
        # For this example, return mock data
        return {
            "city": city,
            "temperature": 72,
            "condition": "sunny",
            "humidity": 45,
            "units": units
        }
    
    # Register calculator tool
    @agent.tool(description="Perform mathematical calculations")
    def calculate(expression: str) -> dict:
        """Evaluate a mathematical expression."""
        print(f"  [Tool called: calculate(expression='{expression}')]")
        
        try:
            # WARNING: eval() is dangerous in production!
            # Use a proper math parser in real applications
            result = eval(expression, {"__builtins__": {}}, {})
            return {"result": result, "expression": expression}
        except Exception as e:
            return {"error": str(e), "expression": expression}
    
    # Register user info tool
    @agent.tool(name="get_user_info", description="Get information about a user")
    def get_user(user_id: int) -> dict:
        """Get user information by ID."""
        print(f"  [Tool called: get_user_info(user_id={user_id})]")
        
        # Mock user database
        users = {
            1: {"name": "Alice", "role": "Engineer", "active": True},
            2: {"name": "Bob", "role": "Designer", "active": True},
            3: {"name": "Charlie", "role": "Manager", "active": False},
        }
        
        return users.get(user_id, {"error": "User not found"})
    
    print("Tools registered:")
    print(f"  - {len(agent._tools)} tools available")
    for tool_name in agent._tools.keys():
        print(f"    • {tool_name}")
    print()
    
    print("=" * 50)
    print("Example queries:")
    print("  - What's the weather in Seattle?")
    print("  - Calculate 42 * 13")
    print("  - What's the weather in Tokyo and London?")
    print("  - Tell me about user 1")
    print("=" * 50)
    print()
    
    # Test queries
    test_queries = [
        "What's the weather in Seattle?",
        "Calculate 15 * 28",
        "Tell me about user 2",
    ]
    
    for query in test_queries:
        print(f"You: {query}")
        print("Assistant: ", end='', flush=True)
        
        try:
            response = agent.generate(query)
            print(response)
            print()
        except Exception as e:
            print(f"\nError: {e}\n")
    
    # Interactive mode
    print("=" * 50)
    print("Interactive mode - Type your own queries (or 'quit' to exit)")
    print("=" * 50)
    print()
    
    try:
        while True:
            user_input = input("You: ")
            
            if user_input.lower() in ["quit", "exit", ""]:
                break
            
            print("Assistant: ", end='', flush=True)
            
            try:
                response = agent.generate(user_input)
                print(response)
                print()
            except Exception as e:
                print(f"\nError: {e}\n")
    
    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
    
    finally:
        print("\nExample complete!")
        agent.close()
        model.close()
    
    return 0


if __name__ == "__main__":
    sys.exit(main())

