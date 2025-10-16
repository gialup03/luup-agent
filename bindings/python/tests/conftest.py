"""
Pytest configuration and fixtures for luup-agent tests.
"""

import pytest
from pathlib import Path


@pytest.fixture(scope="session")
def model_path():
    """
    Fixture providing path to test model.
    
    Returns path to qwen model in models/ directory.
    Skip tests if model not found.
    """
    # Look for model file
    base_path = Path(__file__).parent.parent.parent.parent
    model_file = base_path / "models" / "qwen2-0.5b-instruct-q4_k_m.gguf"
    
    if not model_file.exists():
        pytest.skip(f"Test model not found: {model_file}")
    
    return str(model_file)


@pytest.fixture(scope="session")
def model(model_path):
    """
    Fixture providing a shared Model instance for tests.
    
    Creates model once per test session to save time.
    """
    from luup_agent import Model
    
    model = Model.from_local(
        model_path,
        gpu_layers=-1,
        context_size=2048,
        threads=0,
    )
    
    yield model
    
    # Cleanup
    model.close()


@pytest.fixture
def agent(model):
    """
    Fixture providing a fresh Agent instance for each test.
    """
    from luup_agent import Agent
    
    agent = Agent(
        model,
        system_prompt="You are a helpful test assistant. Be concise.",
        temperature=0.7,
        max_tokens=256,
        enable_tool_calling=True,
        enable_history=True,
    )
    
    yield agent
    
    # Cleanup
    agent.close()

