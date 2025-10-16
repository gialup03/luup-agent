#!/usr/bin/env python3
"""
Built-in Tools Example - Opt-Out Design

This example demonstrates the three built-in productivity tools with opt-out design:
1. Todo List - Task management
2. Notes - Note-taking with tags
3. Auto-Summarization - Automatic conversation summarization

Built-in tools are enabled by default, but can be disabled for lightweight agents.
"""

import sys
from pathlib import Path

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent))

from luup_agent import Model, Agent


def print_separator(title: str):
    """Print a separator with title."""
    print("\n" + "=" * 60)
    print(title)
    print("=" * 60 + "\n")


def example1_default_agent():
    """Example 1: Default agent with built-in tools enabled."""
    print_separator("Example 1: Default Agent (Built-in Tools Enabled)")
    
    # Create model
    model_path = "../../models/qwen2-0.5b-instruct-q4_k_m.gguf"
    print(f"Loading model from: {model_path}")
    model = Model.from_local(model_path, gpu_layers=-1)
    
    # Create agent with default settings (tools enabled)
    agent = Agent(
        model,
        system_prompt=(
            "You are a helpful assistant with built-in productivity tools. "
            "You have access to a todo list, notes system, and auto-summarization. "
            "Help the user manage their tasks and information effectively."
        ),
        temperature=0.7,
        max_tokens=512,
        enable_builtin_tools=True  # Default: enabled
    )
    
    print("✓ Agent created with all built-in tools!")
    print("\nBuilt-in tools available:")
    print("  • todo - Task management (add, list, complete, delete)")
    print("  • notes - Note-taking with tags (create, read, update, delete, search)")
    print("  • summarization - Auto-summarization when context fills\n")
    
    # Example interaction
    queries = [
        "Add a todo to finish the project report by Friday",
        "Create a note about the team meeting with tags: work, important",
        "List all my current todos",
    ]
    
    for query in queries:
        print(f"User: {query}")
        print("Assistant: ", end="")
        try:
            response = agent.generate(query)
            print(response)
        except Exception as e:
            print(f"Error: {e}")
        print()
    
    agent.close()
    model.close()


def example2_lightweight_agent():
    """Example 2: Lightweight agent without built-in tools."""
    print_separator("Example 2: Lightweight Agent (No Built-in Tools)")
    
    # Create model
    model_path = "../../models/qwen2-0.5b-instruct-q4_k_m.gguf"
    print(f"Loading model from: {model_path}")
    model = Model.from_local(model_path, gpu_layers=-1)
    
    # Create lightweight agent (opt-out of built-in tools)
    agent = Agent(
        model,
        system_prompt=(
            "You are a simple assistant focused on answering questions directly "
            "without additional tools."
        ),
        temperature=0.7,
        max_tokens=512,
        enable_tool_calling=False,      # Disable tool calling entirely
        enable_builtin_tools=False       # Opt-out: no built-in tools
    )
    
    print("✓ Lightweight agent created without built-in tools!")
    print("This agent is more resource-efficient.\n")
    
    # Example interaction
    query = "What is 2 + 2?"
    print(f"User: {query}")
    print("Assistant: ", end="")
    try:
        response = agent.generate(query)
        print(response)
    except Exception as e:
        print(f"Error: {e}")
    print()
    
    agent.close()
    model.close()


def example3_manual_registration():
    """Example 3: Manual tool registration with persistent storage."""
    print_separator("Example 3: Manual Registration with Persistent Storage")
    
    # Create model
    model_path = "../../models/qwen2-0.5b-instruct-q4_k_m.gguf"
    print(f"Loading model from: {model_path}")
    model = Model.from_local(model_path, gpu_layers=-1)
    
    # Create agent without default tools
    agent = Agent(
        model,
        system_prompt=(
            "You are a task management assistant with persistent storage. "
            "Help users manage their todos and notes, which are saved to disk."
        ),
        temperature=0.7,
        max_tokens=512,
        enable_builtin_tools=False  # Start without tools
    )
    
    print("Manually registering tools with persistent storage...")
    
    # Register tools with file storage
    try:
        agent.enable_builtin_todo("demo_todos.json")
        print("  ✓ Todo list enabled (storage: demo_todos.json)")
    except Exception as e:
        print(f"  ✗ Failed to enable todo tool: {e}")
    
    try:
        agent.enable_builtin_notes("demo_notes.json")
        print("  ✓ Notes enabled (storage: demo_notes.json)")
    except Exception as e:
        print(f"  ✗ Failed to enable notes tool: {e}")
    
    try:
        agent.enable_builtin_summarization()
        print("  ✓ Auto-summarization enabled")
    except Exception as e:
        print(f"  ✗ Failed to enable summarization: {e}")
    
    print("\n✓ Agent now has persistent storage for todos and notes!")
    print("Data will be saved to demo_todos.json and demo_notes.json\n")
    
    # Example interaction
    queries = [
        "Add a todo to review pull requests",
        "List all my todos",
    ]
    
    for query in queries:
        print(f"User: {query}")
        print("Assistant: ", end="")
        try:
            response = agent.generate(query)
            print(response)
        except Exception as e:
            print(f"Error: {e}")
        print()
    
    agent.close()
    model.close()


def example4_streaming():
    """Example 4: Streaming with built-in tools."""
    print_separator("Example 4: Streaming with Built-in Tools")
    
    # Create model
    model_path = "../../models/qwen2-0.5b-instruct-q4_k_m.gguf"
    print(f"Loading model from: {model_path}")
    model = Model.from_local(model_path, gpu_layers=-1)
    
    # Create agent with tools
    agent = Agent(
        model,
        system_prompt="You are a helpful assistant with productivity tools.",
        enable_builtin_tools=True
    )
    
    print("✓ Agent created with streaming support\n")
    
    # Streaming example
    query = "What todos do I have?"
    print(f"User: {query}")
    print("Assistant: ", end="", flush=True)
    
    try:
        for token in agent.generate_stream(query):
            print(token, end="", flush=True)
        print("\n")
    except Exception as e:
        print(f"\nError: {e}")
    
    agent.close()
    model.close()


def main():
    """Run all examples."""
    print("=" * 60)
    print("luup-agent Built-in Tools Examples")
    print("=" * 60)
    
    try:
        # Run examples
        example1_default_agent()
        example2_lightweight_agent()
        example3_manual_registration()
        example4_streaming()
        
        # Summary
        print_separator("Summary")
        print("Key Takeaways:")
        print("  1. Built-in tools are enabled by default (opt-out design)")
        print("  2. Set enable_builtin_tools=False for lightweight agents")
        print("  3. Manual registration allows persistent storage configuration")
        print("  4. Three tools available: todo, notes, and auto-summarization")
        print("  5. Built-in tools work seamlessly with streaming")
        print("\nAll examples completed successfully!")
        
    except FileNotFoundError as e:
        print(f"\nError: Model file not found - {e}")
        print("Please ensure the model file exists at the specified path.")
        return 1
    except Exception as e:
        print(f"\nError: {e}")
        import traceback
        traceback.print_exc()
        return 1
    
    return 0


if __name__ == "__main__":
    sys.exit(main())

