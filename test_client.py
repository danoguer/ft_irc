#!/usr/bin/env python3
"""Simple test client to test the IRC server networking layer"""

import socket
import sys
import time

def test_basic_connection(host, port):
    """Test basic connection and disconnection"""
    print(f"\n=== Test 1: Basic Connection ===")
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((host, port))
        print(f"✓ Connected to {host}:{port}")
        
        time.sleep(1)
        sock.close()
        print("✓ Disconnected successfully")
        return True
    except Exception as e:
        print(f"✗ Error: {e}")
        return False

def test_single_message(host, port):
    """Test sending a single complete message"""
    print(f"\n=== Test 2: Single Complete Message ===")
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((host, port))
        print(f"✓ Connected")
        
        message = "NICK alice\r\n"
        sock.send(message.encode())
        print(f"✓ Sent: {repr(message)}")
        
        time.sleep(1)
        sock.close()
        print("✓ Test completed")
        return True
    except Exception as e:
        print(f"✗ Error: {e}")
        return False

def test_multiple_messages(host, port):
    """Test sending multiple messages at once"""
    print(f"\n=== Test 3: Multiple Messages ===")
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((host, port))
        print(f"✓ Connected")
        
        messages = "NICK alice\r\nUSER alice 0 * :Alice Smith\r\nJOIN #test\r\n"
        sock.send(messages.encode())
        print(f"✓ Sent 3 messages in one packet")
        
        time.sleep(1)
        sock.close()
        print("✓ Test completed")
        return True
    except Exception as e:
        print(f"✗ Error: {e}")
        return False

def test_partial_message(host, port):
    """Test sending partial messages (buffering test)"""
    print(f"\n=== Test 4: Partial Message Buffering ===")
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((host, port))
        print(f"✓ Connected")
        
        # Send incomplete message
        sock.send(b"NICK al")
        print(f"✓ Sent partial: 'NICK al' (no \\r\\n)")
        time.sleep(0.5)
        
        # Complete the message
        sock.send(b"ice\r\n")
        print(f"✓ Sent completion: 'ice\\r\\n'")
        time.sleep(1)
        
        sock.close()
        print("✓ Test completed")
        return True
    except Exception as e:
        print(f"✗ Error: {e}")
        return False

def test_mixed_messages(host, port):
    """Test mixed complete and partial messages"""
    print(f"\n=== Test 5: Mixed Complete/Partial Messages ===")
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((host, port))
        print(f"✓ Connected")
        
        # Send complete + partial
        sock.send(b"NICK alice\r\nUSER ali")
        print(f"✓ Sent: complete NICK + partial USER")
        time.sleep(0.5)
        
        # Complete the partial
        sock.send(b"ce 0 * :Alice\r\n")
        print(f"✓ Sent: completion of USER")
        time.sleep(1)
        
        sock.close()
        print("✓ Test completed")
        return True
    except Exception as e:
        print(f"✗ Error: {e}")
        return False

def test_multiple_clients(host, port):
    """Test multiple simultaneous clients"""
    print(f"\n=== Test 6: Multiple Clients ===")
    try:
        clients = []
        
        # Connect 3 clients
        for i in range(3):
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((host, port))
            clients.append(sock)
            print(f"✓ Client {i+1} connected")
        
        time.sleep(0.5)
        
        # Each client sends a message
        for i, sock in enumerate(clients):
            message = f"NICK user{i+1}\r\n"
            sock.send(message.encode())
            print(f"✓ Client {i+1} sent: {repr(message)}")
        
        time.sleep(1)
        
        # Close all clients
        for i, sock in enumerate(clients):
            sock.close()
            print(f"✓ Client {i+1} disconnected")
        
        print("✓ Test completed")
        return True
    except Exception as e:
        print(f"✗ Error: {e}")
        return False

def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <host> <port>")
        print(f"Example: {sys.argv[0]} localhost 6667")
        sys.exit(1)
    
    host = sys.argv[1]
    port = int(sys.argv[2])
    
    print(f"Testing IRC Server at {host}:{port}")
    print("=" * 50)
    
    tests = [
        test_basic_connection,
        test_single_message,
        test_multiple_messages,
        test_partial_message,
        test_mixed_messages,
        test_multiple_clients
    ]
    
    passed = 0
    for test in tests:
        if test(host, port):
            passed += 1
        time.sleep(1)  # Wait between tests
    
    print(f"\n{'=' * 50}")
    print(f"Results: {passed}/{len(tests)} tests passed")
    print("\nCheck server output to see message extraction working!")

if __name__ == "__main__":
    main()
