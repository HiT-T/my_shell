#!/bin/bash
# test1.sh

# Ensure this file is executable with: chmod +x test1.sh

echo "===== Test 1: Basic External & Built-ins ====="

# 1. Simple external commands
echo "Running 'echo' and 'pwd':"
echo "Hello from test1.sh"
pwd

# 2. Built-in: cd (assumes 'subdir' is valid; adjust if needed)
echo "Attempting to 'cd subdir'..."
cd subdir
pwd

# 3. Built-in: which
echo "Which command for 'ls':"
which ls

# 4. Built-in: exit
echo "The next line won't run if exit works properly."
exit
echo "If you see this line, 'exit' didn't stop the script."