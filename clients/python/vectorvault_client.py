"""
VectorVault Python Client

Simple REST API client for VectorVault vector search engine.
"""

import requests
from typing import List, Dict, Any, Optional


class VectorVaultClient:
    """
    Python client for VectorVault REST API
    
    Args:
        host: Server hostname (default: localhost)
        port: Server port (default: 8080)
        timeout: Request timeout in seconds (default: 30)
    
    Example:
        >>> client = VectorVaultClient()
        >>> client.add(id=1, vec=[0.1, 0.2, 0.3, ...])
        >>> results = client.search(vec=[0.15, 0.25, ...], k=10)
    """
    
    def __init__(self, host: str = "localhost", port: int = 8080, timeout: int = 30):
        self.base_url = f"http://{host}:{port}"
        self.timeout = timeout
        self.session = requests.Session()
    
    def add(self, id: int, vec: List[float]) -> Dict[str, Any]:
        """
        Add a vector to the index
        
        Args:
            id: Unique vector ID
            vec: Vector as list of floats
            
        Returns:
            Response dictionary with status
            
        Raises:
            requests.HTTPError: If request fails
        """
        response = self.session.post(
            f"{self.base_url}/add",
            json={"id": id, "vec": vec},
            timeout=self.timeout
        )
        response.raise_for_status()
        return response.json()
    
    def search(self, vec: List[float], k: int = 10, ef: int = 50) -> Dict[str, Any]:
        """
        Search for k nearest neighbors
        
        Args:
            vec: Query vector as list of floats
            k: Number of results to return (default: 10)
            ef: Search quality parameter (default: 50)
            
        Returns:
            Response dictionary with results and latency
            
        Raises:
            requests.HTTPError: If request fails
        """
        response = self.session.post(
            f"{self.base_url}/query",
            params={"k": k, "ef": ef},
            json={"vec": vec},
            timeout=self.timeout
        )
        response.raise_for_status()
        return response.json()
    
    def save(self, path: str) -> Dict[str, Any]:
        """
        Save index to disk
        
        Args:
            path: File path to save index
            
        Returns:
            Response dictionary with status
        """
        response = self.session.post(
            f"{self.base_url}/save",
            json={"path": path},
            timeout=self.timeout
        )
        response.raise_for_status()
        return response.json()
    
    def load(self, path: str) -> Dict[str, Any]:
        """
        Load index from disk
        
        Args:
            path: File path to load index from
            
        Returns:
            Response dictionary with index info
        """
        response = self.session.post(
            f"{self.base_url}/load",
            json={"path": path},
            timeout=self.timeout
        )
        response.raise_for_status()
        return response.json()
    
    def stats(self) -> Dict[str, Any]:
        """
        Get index statistics
        
        Returns:
            Response dictionary with index stats
        """
        response = self.session.get(
            f"{self.base_url}/stats",
            timeout=self.timeout
        )
        response.raise_for_status()
        return response.json()
    
    def health(self) -> Dict[str, Any]:
        """
        Check server health
        
        Returns:
            Response dictionary with health status
        """
        response = self.session.get(
            f"{self.base_url}/health",
            timeout=self.timeout
        )
        response.raise_for_status()
        return response.json()
    
    def close(self):
        """Close the HTTP session"""
        self.session.close()
    
    def __enter__(self):
        """Context manager entry"""
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit"""
        self.close()
        return False


__all__ = ['VectorVaultClient']