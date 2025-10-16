"""
Tests for Model class.
"""

import pytest
from pathlib import Path

from luup_agent import Model, ModelNotFoundError, BackendInitError


def test_model_not_found():
    """Test that ModelNotFoundError is raised for nonexistent file."""
    with pytest.raises((ModelNotFoundError, FileNotFoundError)):
        Model.from_local("nonexistent_model.gguf")


def test_model_creation(model_path):
    """Test model creation from local file."""
    model = Model.from_local(
        model_path,
        gpu_layers=0,  # CPU only for reliable test
        context_size=512,
        threads=1,
    )
    
    assert model is not None
    assert not model._closed
    
    model.close()
    assert model._closed


def test_model_context_manager(model_path):
    """Test model as context manager."""
    with Model.from_local(model_path, gpu_layers=0) as model:
        assert not model._closed
        info = model.get_info()
        assert isinstance(info, dict)
    
    # Should be closed after exiting context
    assert model._closed


def test_model_get_info(model):
    """Test getting model information."""
    info = model.get_info()
    
    assert isinstance(info, dict)
    assert "backend" in info
    assert "device" in info
    assert "gpu_layers_loaded" in info
    assert "memory_usage" in info
    assert "context_size" in info
    
    # Check types
    assert isinstance(info["backend"], str)
    assert isinstance(info["device"], str)
    assert isinstance(info["gpu_layers_loaded"], int)
    assert isinstance(info["memory_usage"], int)
    assert isinstance(info["context_size"], int)
    
    # Check values
    assert len(info["backend"]) > 0
    assert info["context_size"] > 0


def test_model_warmup(model):
    """Test model warmup."""
    # Should not raise
    model.warmup()


def test_model_double_close(model_path):
    """Test that closing model twice is safe."""
    model = Model.from_local(model_path, gpu_layers=0)
    
    model.close()
    assert model._closed
    
    # Should not raise
    model.close()
    assert model._closed


def test_model_operations_after_close(model_path):
    """Test that operations fail after model is closed."""
    model = Model.from_local(model_path, gpu_layers=0)
    model.close()
    
    with pytest.raises(ValueError, match="closed"):
        model.get_info()
    
    with pytest.raises(ValueError, match="closed"):
        model.warmup()


def test_model_repr(model):
    """Test model string representation."""
    repr_str = repr(model)
    
    assert "Model" in repr_str
    assert "open" in repr_str or "closed" in repr_str

