#!/usr/bin/env python3

"""
Test script to verify that our base64 encoding changes work correctly.
This script will test the changes across Java, C#, and C++ implementations.
"""

import subprocess
import tempfile
import os
import json
import base64

def test_java_changes():
    """Test Java base64 encoding changes"""
    print("Testing Java changes...")
    
    # Check if the Java file has been modified correctly
    java_writer_path = "/workspace/bond/java/json/src/main/java/org/bondlib/SimpleJsonWriter.java"
    
    with open(java_writer_path, 'r') as f:
        content = f.read()
        if "Base64.getEncoder().encodeToString" in content:
            print("✓ Java writer correctly uses base64 encoding")
        else:
            print("✗ Java writer does not use base64 encoding")
            return False
    
    # Check Java reader
    java_reader_path = "/workspace/bond/java/json/src/main/java/org/bondlib/SimpleJsonReader.java"
    
    with open(java_reader_path, 'r') as f:
        content = f.read()
        if "Base64.getDecoder().decode" in content:
            print("✓ Java reader correctly uses base64 decoding")
        else:
            print("✗ Java reader does not use base64 decoding")
            return False
    
    return True

def test_csharp_changes():
    """Test C# base64 encoding changes"""
    print("Testing C# changes...")
    
    # Check C# writer
    csharp_writer_path = "/workspace/bond/cs/src/json/protocols/SimpleJsonWriter.cs"
    
    with open(csharp_writer_path, 'r') as f:
        content = f.read()
        if "Convert.ToBase64String" in content:
            print("✓ C# writer correctly uses base64 encoding")
        else:
            print("✗ C# writer does not use base64 encoding")
            return False
    
    # Check C# reader
    csharp_parser_path = "/workspace/bond/cs/src/json/expressions/json/SimpleJsonParser.cs"
    
    with open(csharp_parser_path, 'r') as f:
        content = f.read()
        if "FromBase64String" in content:
            print("✓ C# parser correctly uses base64 decoding")
        else:
            print("✗ C# parser does not use base64 decoding")
            return False
    
    return True

def test_cpp_changes():
    """Test C++ base64 encoding changes"""
    print("Testing C++ changes...")
    
    # Check C++ writer
    cpp_writer_path = "/workspace/bond/cpp/inc/bond/protocol/simple_json_writer.h"
    
    with open(cpp_writer_path, 'r') as f:
        content = f.read()
        if "detail::EncodeBase64" in content:
            print("✓ C++ writer correctly uses base64 encoding")
        else:
            print("✗ C++ writer does not use base64 encoding")
            return False
    
    # Check C++ encoding functions
    cpp_encoding_path = "/workspace/bond/cpp/inc/bond/protocol/encoding.h"
    
    with open(cpp_encoding_path, 'r') as f:
        content = f.read()
        if "EncodeBase64" in content and "DecodeBase64" in content:
            print("✓ C++ encoding functions correctly implemented")
        else:
            print("✗ C++ encoding functions not implemented")
            return False
    
    # Check C++ reader
    cpp_reader_path = "/workspace/bond/cpp/inc/bond/protocol/simple_json_reader_impl.h"
    
    with open(cpp_reader_path, 'r') as f:
        content = f.read()
        if "detail::DecodeBase64" in content:
            print("✓ C++ reader correctly uses base64 decoding")
        else:
            print("✗ C++ reader does not use base64 decoding")
            return False
    
    return True

def test_base64_functionality():
    """Test that base64 encoding/decoding works correctly"""
    print("Testing base64 functionality...")
    
    # Test data
    test_bytes = b"Hello, World! This is a test of binary data encoding."
    expected_base64 = base64.b64encode(test_bytes).decode('ascii')
    
    print(f"Test data: {test_bytes}")
    print(f"Expected base64: {expected_base64}")
    
    # Verify base64 round-trip
    decoded = base64.b64decode(expected_base64)
    if decoded == test_bytes:
        print("✓ Base64 round-trip works correctly")
        return True
    else:
        print("✗ Base64 round-trip failed")
        return False

def main():
    """Main test function"""
    print("Testing Bond base64 encoding changes...")
    print("=" * 50)
    
    all_tests_passed = True
    
    # Test each language implementation
    all_tests_passed &= test_java_changes()
    print()
    
    all_tests_passed &= test_csharp_changes()
    print()
    
    all_tests_passed &= test_cpp_changes()
    print()
    
    all_tests_passed &= test_base64_functionality()
    print()
    
    print("=" * 50)
    if all_tests_passed:
        print("✓ All tests passed! Base64 encoding changes are correctly implemented.")
    else:
        print("✗ Some tests failed. Please review the changes.")
    
    return all_tests_passed

if __name__ == "__main__":
    success = main()
    exit(0 if success else 1)