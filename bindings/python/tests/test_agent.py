"""
Tests for Agent class.
"""

import pytest
import asyncio

from luup_agent import Agent


def test_agent_creation(agent):
    """Test agent creation."""
    assert agent is not None
    assert not agent._closed
    assert len(agent._tools) == 0


def test_agent_context_manager(model):
    """Test agent as context manager."""
    with Agent(model, system_prompt="Test assistant") as agent:
        assert not agent._closed
        history = agent.get_history()
        assert isinstance(history, list)
    
    # Should be closed after exiting context
    assert agent._closed


def test_agent_generate(agent):
    """Test blocking generation."""
    response = agent.generate("Say hello")
    
    assert isinstance(response, str)
    assert len(response) > 0


def test_agent_generate_stream(agent):
    """Test streaming generation."""
    tokens = list(agent.generate_stream("Count to 3"))
    
    assert len(tokens) > 0
    assert all(isinstance(token, str) for token in tokens)
    
    # Concatenated tokens should form valid response
    full_response = ''.join(tokens)
    assert len(full_response) > 0


@pytest.mark.asyncio
async def test_agent_generate_async(agent):
    """Test async streaming generation."""
    tokens = []
    
    async for token in agent.generate_async("Say hi"):
        assert isinstance(token, str)
        tokens.append(token)
    
    assert len(tokens) > 0
    
    # Concatenated tokens should form valid response
    full_response = ''.join(tokens)
    assert len(full_response) > 0


def test_agent_history(agent):
    """Test conversation history management."""
    # Initially empty (or just system prompt)
    history = agent.get_history()
    initial_len = len(history)
    
    # Add user message
    agent.add_message("user", "Hello")
    history = agent.get_history()
    assert len(history) == initial_len + 1
    assert history[-1]["role"] == "user"
    assert history[-1]["content"] == "Hello"
    
    # Add assistant message
    agent.add_message("assistant", "Hi there!")
    history = agent.get_history()
    assert len(history) == initial_len + 2
    assert history[-1]["role"] == "assistant"
    assert history[-1]["content"] == "Hi there!"
    
    # Clear history
    agent.clear_history()
    history = agent.get_history()
    # Should be back to initial state (empty or just system prompt)
    assert len(history) == initial_len


def test_agent_close(model):
    """Test agent cleanup."""
    agent = Agent(model, system_prompt="Test")
    
    assert not agent._closed
    
    agent.close()
    assert agent._closed
    
    # Operations should fail after close
    with pytest.raises(ValueError, match="closed"):
        agent.generate("test")


def test_agent_repr(agent):
    """Test agent string representation."""
    repr_str = repr(agent)
    
    assert "Agent" in repr_str
    assert "open" in repr_str or "closed" in repr_str
    assert "tools=" in repr_str

