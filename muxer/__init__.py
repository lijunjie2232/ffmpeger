# avmerger/__init__.py
"""
Audio Video Merger package
"""

try:
    from .avmerger import AudioVideoMerger
except ImportError as e:
    raise ImportError(f"Failed to import avmerger extension: {e}")

__version__ = "0.1.0"
__author__ = "Your Name"

__all__ = ['AudioVideoMerger']
